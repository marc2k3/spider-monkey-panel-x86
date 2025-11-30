#include <stdafx.h>
#include "js_error_helper.h"

#include <convert/js_to_native.h>
#include <convert/native_to_js.h>
#include <js_utils/cached_utf8_paths_hack.h>
#include <js_utils/js_property_helper.h>

using namespace smp;

namespace
{
	using namespace mozjs;

	std::string GetStackTraceString(JSContext* cx, JS::HandleObject exn)
	{
		try
		{ // Must not throw errors in error handler
			// Note: exceptions thrown while compiling top-level script have no stack.
			JS::RootedObject stackObj(cx, JS::ExceptionStackOrNull(exn));
			if (!stackObj)
			{ // quack?
				return GetOptionalProperty<std::string>(cx, exn, "stack").value_or("");
			}

			JS::RootedString stackStr(cx);
			if (!JS::BuildStackString(cx, nullptr, stackObj, &stackStr, 2))
			{
				return "";
			}

			const auto& cachedPaths = hack::GetAllCachedUtf8Paths();
			auto pfcStackStr = mozjs::convert::to_native::ToValue<pfc::string>(cx, stackStr);

			for (const auto& [id, path] : cachedPaths)
			{
				const auto filename = pfc::string_filename_ext(path.c_str());
				pfcStackStr = pfcStackStr.replace(id.c_str(), filename.c_str());
			}

			return pfcStackStr.get_ptr();
		}
		catch (...)
		{
			mozjs::error::SuppressException(cx);
			return "";
		}
	}

	bool PrependTextToJsStringException(JSContext* cx, JS::HandleValue excn, const std::string& text)
	{
		std::string currentMessage;
		try
		{ // Must not throw errors in error handler
			currentMessage = convert::to_native::ToValue<std::string>(cx, excn);
		}
		catch (const JsException&)
		{
			return false;
		}
		catch (const QwrException&)
		{
			return false;
		}

		if (currentMessage == std::string("out of memory"))
		{ // Can't modify the message since we're out of memory
			return true;
		}

		const std::string newMessage = [&text, &currentMessage] {
			std::string newMessage;
			newMessage += text;
			if (currentMessage.length())
			{
				newMessage += (":\n");
				newMessage += currentMessage;
			}

			return newMessage;
			}();

		JS::RootedValue jsMessage(cx);
		try
		{ // Must not throw errors in error handler
			convert::to_js::ToValue<std::string>(cx, newMessage, &jsMessage);
		}
		catch (const JsException&)
		{
			return false;
		}
		catch (const QwrException&)
		{
			return false;
		}

		JS_SetPendingException(cx, jsMessage);
		return true;
	}

	bool PrependTextToJsObjectException(JSContext* cx, JS::HandleValue excn, const std::string& text)
	{
		auto autoClearOnError = wil::scope_exit([cx] { JS_ClearPendingException(cx); });

		JS::RootedObject excnObject(cx, &excn.toObject());
		JS_ClearPendingException(cx); ///< need this for js::ErrorReport::init

		JS::RootedObject excnStackObject(cx, JS::ExceptionStackOrNull(excnObject));
		if (!excnStackObject)
		{ // Sometimes happens with custom JS errors
			return false;
		}

		JS::ExceptionStack excnStack(cx, excn, excnStackObject);

		JS::ErrorReportBuilder reportBuilder(cx);
		if (!reportBuilder.init(cx, excnStack, JS::ErrorReportBuilder::SniffingBehavior::WithSideEffects))
		{ // Sometimes happens with custom JS errors
			return false;
		}

		JSErrorReport* pReport = reportBuilder.report();

		std::string currentMessage = pReport->message().c_str();
		const std::string newMessage = [&text, &currentMessage] {
			std::string newMessage;
			newMessage += text;
			if (currentMessage.length())
			{
				newMessage += (":\n");
				newMessage += currentMessage;
			}

			return newMessage;
			}();

		JS::RootedValue jsFilename(cx);
		JS::RootedValue jsMessage(cx);
		try
		{ // Must not throw errors in error handler
			convert::to_js::ToValue<std::string>(cx, pReport->filename, &jsFilename);
			convert::to_js::ToValue<std::string>(cx, newMessage, &jsMessage);
		}
		catch (...)
		{
			mozjs::error::SuppressException(cx);
			return false;
		}

		JS::RootedValue newExcn(cx);
		JS::RootedString jsFilenameStr(cx, jsFilename.toString());
		JS::RootedString jsMessageStr(cx, jsMessage.toString());

		if (!JS_WrapObject(cx, &excnStackObject))
		{ // Need wrapping for the case when exception is thrown from internal global
			return false;
		}

		if (!JS::CreateError(cx, static_cast<JSExnType>(pReport->exnType), excnStackObject, jsFilenameStr, pReport->lineno, pReport->column, nullptr, jsMessageStr, &newExcn))
		{
			return false;
		}

		autoClearOnError.release();
		JS_SetPendingException(cx, newExcn);
		return true;
	}
}

namespace mozjs::error
{
	AutoJsReport::AutoJsReport(JSContext* cx) : cx(cx) {}

	AutoJsReport::~AutoJsReport() noexcept
	{
		if (isDisabled_)
		{
			return;
		}

		if (!JS_IsExceptionPending(cx))
		{
			return;
		}

		try
		{
			std::string errorText = JsErrorToText(cx);
			JS_ClearPendingException(cx);

			JS::RootedObject global(cx, JS::CurrentGlobalOrNull(cx));
			if (!global)
			{
				assert(0);
				return;
			}

			auto globalCtx = static_cast<JsGlobalObject*>(JS::GetPrivate(global));
			if (!globalCtx)
			{
				assert(0);
				return;
			}

			globalCtx->Fail(errorText);
			JS_ClearPendingException(cx);
		}
		catch (const std::exception&)
		{
		}
	}

	void AutoJsReport::Disable()
	{
		isDisabled_ = true;
	}

	std::string JsErrorToText(JSContext* cx)
	{
		assert(JS_IsExceptionPending(cx));

		JS::RootedValue excn(cx);
		(void)JS_GetPendingException(cx, &excn);
		JS_ClearPendingException(cx); ///< need this for js::ErrorReport::init

		auto autoErrorClear = wil::scope_exit([cx]()
			{ // There should be no exceptions on function exit
				JS_ClearPendingException(cx);
			});

		std::string errorText;
		if (excn.isString())
		{
			try
			{ // Must not throw errors in error handler
				errorText = convert::to_native::ToValue<std::string>(cx, excn);
			}
			catch (...)
			{
				mozjs::error::SuppressException(cx);
			}
		}
		else if (excn.isObject())
		{
			JS::RootedObject excnObject(cx, &excn.toObject());

			JSErrorReport* pReport = JS_ErrorFromException(cx, excnObject);
			if (!pReport)
			{ // Sometimes happens with custom JS errors
				return errorText;
			}

			assert(!pReport->isWarning());

			errorText = pReport->message().c_str();

			const auto additionalInfo = [&] {
				std::string additionalInfo;

				if (pReport->filename)
				{
					const auto stdPath = [&]() -> std::string {
						if (std::string(pReport->filename) == "")
						{
							return "<main>";
						}

						const auto pathOpt = hack::GetCachedUtf8Path(pReport->filename);
						if (pathOpt)
						{
							return pathOpt->filename().u8string();
						}

						return pReport->filename;
						}();

					additionalInfo += "\n";
					additionalInfo += fmt::format("File: {}\n", stdPath);
					additionalInfo += fmt::format("Line: {}, Column: {}", std::to_string(pReport->lineno), std::to_string(pReport->column));
					if (pReport->linebufLength())
					{
						additionalInfo += "\n";
						additionalInfo += "Source: ";
						additionalInfo += [pReport] {
							pfc::string8 tmpBuf = pfc::stringcvt::string_utf8_from_utf16(pReport->linebuf(), pReport->linebufLength()).get_ptr();
							tmpBuf.truncate_eol();
							return tmpBuf;
							}();
					}
				}

				if (excn.isObject())
				{
					JS::RootedObject excnObject(cx, &excn.toObject());
					std::string stackTrace = GetStackTraceString(cx, excnObject);
					if (!stackTrace.empty())
					{
						additionalInfo += "\n";
						additionalInfo += "Stack trace:\n";
						additionalInfo += stackTrace;
					}
				}

				return additionalInfo;
				}();

			if (!additionalInfo.empty())
			{
				errorText += "\n";
				errorText += additionalInfo;
			}
		}

		return errorText;
	}

	void ExceptionToJsError(JSContext* cx)
	{
		try
		{
			throw;
		}
		catch (const JsException&)
		{
			assert(JS_IsExceptionPending(cx));
		}
		catch (const QwrException& e)
		{
			JS_ClearPendingException(cx);
			JS_ReportErrorUTF8(cx, e.what());
		}
		catch (const _com_error& e)
		{
			JS_ClearPendingException(cx);

			const auto errorMsg8 = qwr::ToU8(std::wstring_view{ e.ErrorMessage() ? e.ErrorMessage() : L"<none>" });
			const auto errorSource8 = qwr::ToU8(std::wstring_view{ e.Source().length() ? static_cast<const wchar_t*>(e.Source()) : L"<none>" });
			const auto errorDesc8 = qwr::ToU8(std::wstring_view{ e.Description().length() ? static_cast<const wchar_t*>(e.Description()) : L"<none>" });
			JS_ReportErrorUTF8(
				cx,
				fmt::format(
					"COM error:\n"
					"  hresult: {:#x}\n"
					"  message: {}\n"
					"  description: {}\n"
					"  source: {}",
					static_cast<uint32_t>(e.Error()),
					errorMsg8,
					errorDesc8,
					errorSource8
				).c_str()
			);
		}
		catch (const std::bad_alloc&)
		{
			JS_ClearPendingException(cx);
			JS_ReportAllocationOverflow(cx);
		}
		// SM is not designed to handle uncaught exceptions, so we are risking here,
		// hoping that this exception will reach fb2k handler.
	}

	std::string ExceptionToText(JSContext* cx)
	{
		try
		{
			throw;
		}
		catch (const JsException&)
		{
			return JsErrorToText(cx);
		}
		catch (const QwrException& e)
		{
			JS_ClearPendingException(cx);
			return e.what();
		}
		catch (const _com_error& e)
		{
			JS_ClearPendingException(cx);

			const auto errorMsg8 = qwr::ToU8(std::wstring_view{ e.ErrorMessage() ? e.ErrorMessage() : L"<none>" });
			const auto errorSource8 = qwr::ToU8(std::wstring_view{ e.Source().length() ? static_cast<const wchar_t*>(e.Source()) : L"<none>" });
			const auto errorDesc8 = qwr::ToU8(std::wstring_view{ e.Description().length() ? static_cast<const wchar_t*>(e.Description()) : L"<none>" });
			return fmt::format("COM error:\n"
				"  hresult: {:#x}\n"
				"  message: {}\n"
				"  description: {}\n"
				"  source: {}",
				static_cast<uint32_t>(e.Error()),
				errorMsg8,
				errorDesc8,
				errorSource8);
		}
		catch (const std::bad_alloc& e)
		{
			JS_ClearPendingException(cx);
			return e.what();
		}
		// SM is not designed to handle uncaught exceptions, so we are risking here,
		// hoping that this exception will reach fb2k handler.
	}

	void SuppressException(JSContext* cx)
	{
		try
		{
			throw;
		}
		catch (const JsException&)
		{
		}
		catch (const QwrException&)
		{
		}
		catch (const _com_error&)
		{
		}
		catch (const std::bad_alloc&)
		{
		}
		// SM is not designed to handle uncaught exceptions, so we are risking here,
		// hoping that this exception will reach fb2k handler.

		JS_ClearPendingException(cx);
	}

	void PrependTextToJsError(JSContext* cx, const std::string& text)
	{
		auto autoJsReport = wil::scope_exit([cx, text]
			{
				JS_ReportErrorUTF8(cx, "%s", text.c_str());
			});

		if (!JS_IsExceptionPending(cx))
		{
			return;
		}

		// Get exception object before printing and clearing exception.
		JS::RootedValue excn(cx);
		(void)JS_GetPendingException(cx, &excn);

		if (excn.isString())
		{
			if (PrependTextToJsStringException(cx, excn, text))
			{
				autoJsReport.release();
			}
			return;
		}
		else if (excn.isObject())
		{
			if (PrependTextToJsObjectException(cx, excn, text))
			{
				autoJsReport.release();
			}
			return;
		}
	}
}
