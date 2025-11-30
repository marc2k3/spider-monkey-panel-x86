#pragma once
#include <fb2k/advanced_config.h>

namespace mozjs::error
{
	[[nodiscard]] std::string JsErrorToText(JSContext* cx);
	void ExceptionToJsError(JSContext* cx);
	[[nodiscard]] std::string ExceptionToText(JSContext* cx);
	void SuppressException(JSContext* cx);
	void PrependTextToJsError(JSContext* cx, const std::string& text);

	template <typename F, typename... Args>
	[[nodiscard]] bool Execute_JsSafe(JSContext* cx, std::string_view functionName, F&& func, Args&&... args)
	{
		try
		{
			func(cx, std::forward<Args>(args)...);
		}
		catch (...)
		{
			mozjs::error::ExceptionToJsError(cx);
		}

		if (JS_IsExceptionPending(cx))
		{
			mozjs::error::PrependTextToJsError(cx, fmt::format("{} failed", functionName));
			return false;
		}

		return true;
	}

	class AutoJsReport
	{
	public:
		explicit [[nodiscard]] AutoJsReport(JSContext* cx);
		~AutoJsReport() noexcept;

		void Disable();

	private:
		JSContext* cx{};
		bool isDisabled_ = false;
	};
}
