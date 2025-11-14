#include <stdafx.h>
#include "console.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_object_helper.h>

using namespace smp;

namespace
{
	constexpr uint32_t kMaxLogDepth = 20;

	using namespace mozjs;

	std::string ParseJsValue(JSContext* ctx, JS::HandleValue jsValue, JS::MutableHandleObjectVector curObjects, uint32_t& logDepth, bool isParentObject);

	std::string ParseJsArray(JSContext* ctx, JS::HandleObject jsObject, JS::MutableHandleObjectVector curObjects, uint32_t& logDepth)
	{
		std::string output;

		output += "[";

		uint32_t arraySize;
		if (!JS::GetArrayLength(ctx, jsObject, &arraySize))
		{
			throw JsException();
		}

		JS::RootedValue arrayElement(ctx);
		for (uint32_t i = 0; i < arraySize; ++i)
		{
			if (!JS_GetElement(ctx, jsObject, i, &arrayElement))
			{
				throw JsException();
			}

			output += ParseJsValue(ctx, arrayElement, curObjects, logDepth, true);
			if (i != arraySize - 1)
			{
				output += ", ";
			}
		}

		output += "]";

		return output;
	}

	std::string ParseJsObject(JSContext* ctx, JS::HandleObject jsObject, JS::MutableHandleObjectVector curObjects, uint32_t& logDepth)
	{
		std::string output;

		{
			JS::RootedObject jsUnwrappedObject(ctx, jsObject);
			if (js::IsWrapper(jsObject))
			{
				jsUnwrappedObject = js::UncheckedUnwrap(jsObject);
			}
			if (js::IsProxy(jsUnwrappedObject) && js::GetProxyHandler(jsUnwrappedObject)->family() == GetSmpProxyFamily())
			{
				jsUnwrappedObject = js::GetProxyTargetObject(jsUnwrappedObject);
			}

			output += JS::InformalValueTypeName(JS::ObjectValue(*jsUnwrappedObject));
		}
		output += " {";

		JS::RootedIdVector jsVector(ctx);
		if (!js::GetPropertyKeys(ctx, jsObject, 0, &jsVector))
		{
			throw JsException();
		}

		JS::RootedValue jsIdValue(ctx);
		JS::RootedValue jsValue(ctx);
		bool hasFunctions = false;
		for (size_t i = 0, length = jsVector.length(); i < length; ++i)
		{
			const auto& jsId = jsVector[i];
			if (!JS_GetPropertyById(ctx, jsObject, jsId, &jsValue))
			{
				throw JsException();
			}

			if (jsValue.isObject() && JS_ObjectIsFunction(&jsValue.toObject()))
			{
				hasFunctions = true;
			}
			else
			{
				jsIdValue = js::IdToValue(jsId);
				output += convert::to_native::ToValue<std::string>(ctx, jsIdValue);
				output += "=";
				output += ParseJsValue(ctx, jsValue, curObjects, logDepth, true);
				if (i != length - 1 || hasFunctions)
				{
					output += ", ";
				}
			}
		}

		if (hasFunctions)
		{
			output += "...";
		}

		output += "}";

		return output;
	}

	std::string ParseJsValue(JSContext* ctx, JS::HandleValue jsValue, JS::MutableHandleObjectVector curObjects, uint32_t& logDepth, bool isParentObject)
	{
		std::string output;

		++logDepth;
		auto autoDecrement = wil::scope_exit([&logDepth] { --logDepth; });

		if (!jsValue.isObject())
		{
			const bool showQuotes = isParentObject && jsValue.isString();

			if (showQuotes)
			{
				output += "\"";
			}
			output += convert::to_native::ToValue<std::string>(ctx, jsValue);
			if (showQuotes)
			{
				output += "\"";
			}
		}
		else
		{
			if (logDepth > kMaxLogDepth)
			{ // Don't parse object, if we reached the depth limit
				output += JS::InformalValueTypeName(jsValue);
				return output;
			}

			JS::RootedObject jsObject(ctx, &jsValue.toObject());

			if (JS_ObjectIsFunction(jsObject))
			{
				output += JS::InformalValueTypeName(jsValue);
			}
			else
			{
				for (const auto& curObject : curObjects)
				{
					if (jsObject.get() == curObject)
					{
						output += "<Circular>";
						return output;
					}
				}

				std::ignore = curObjects.emplaceBack(jsObject);
				auto autoPop = wil::scope_exit([&curObjects] { curObjects.popBack(); });

				bool is;
				if (!JS::IsArrayObject(ctx, jsObject, &is))
				{
					throw JsException();
				}

				if (is)
				{
					output += ParseJsArray(ctx, jsObject, curObjects, logDepth);
				}
				else
				{
					output += ParseJsObject(ctx, jsObject, curObjects, logDepth);
				}
			}
		}

		return output;
	}

	std::optional<std::string> ParseLogArgs(JSContext* ctx, JS::CallArgs& args)
	{
		if (!args.length())
			return std::nullopt;

		Strings parts;
		JS::RootedObjectVector curObjects(ctx);
		uint32_t logDepth{};

		for (size_t i = 0; i < args.length(); ++i)
		{
			const auto part = ParseJsValue(ctx, args[i], &curObjects, logDepth, false);
			parts.emplace_back(part);
		}

		return fmt::format("{}", fmt::join(parts, " "));
	}

	bool LogImpl(JSContext* ctx, unsigned argc, JS::Value* vp)
	{
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

		auto output = ParseLogArgs(ctx, args);
		args.rval().setUndefined();

		if (output)
			console::info(output->c_str());

		return true;
	}

	MJS_DEFINE_JS_FN(Log, LogImpl)

	constexpr auto console_functions = std::to_array<JSFunctionSpec>(
		{
			JS_FN("log", Log, 0, kDefaultPropsFlags),
			JS_FS_END,
		});
}

namespace mozjs
{
	void DefineConsole(JSContext* ctx, JS::HandleObject global)
	{
		JS::RootedObject consoleObj(ctx, JS_NewPlainObject(ctx));
		if (!consoleObj
			|| !JS_DefineFunctions(ctx, consoleObj, console_functions.data())
			|| !JS_DefineProperty(ctx, global, "console", consoleObj, kDefaultPropsFlags))
		{
			throw JsException();
		}
	}
}
