#include <stdafx.h>
#include "js_to_native.h"

namespace mozjs::convert::to_native
{
	namespace internal
	{
		template <>
		bool ToSimpleValue(JSContext*, const JS::HandleValue& jsValue)
		{
			return JS::ToBoolean(jsValue);
		}

		template <>
		int8_t ToSimpleValue(JSContext* cx, const JS::HandleValue& jsValue)
		{
			int8_t val{};

			if (JS::ToInt8(cx, jsValue, &val))
			{
				return val;
			}

			throw smp::JsException();
		}

		template <>
		int32_t ToSimpleValue(JSContext* cx, const JS::HandleValue& jsValue)
		{
			int32_t val{};

			if (JS::ToInt32(cx, jsValue, &val))
			{
				return val;
			}

			throw smp::JsException();
		}

		template <>
		uint8_t ToSimpleValue(JSContext* cx, const JS::HandleValue& jsValue)
		{
			uint8_t val{};

			if (JS::ToUint8(cx, jsValue, &val))
			{
				return val;
			}

			throw smp::JsException();
		}

		template <>
		uint32_t ToSimpleValue(JSContext* cx, const JS::HandleValue& jsValue)
		{
			uint32_t val{};

			if (JS::ToUint32(cx, jsValue, &val))
			{
				return val;
			}

			throw smp::JsException();
		}

		template <>
		int64_t ToSimpleValue(JSContext* cx, const JS::HandleValue& jsValue)
		{
			int64_t val{};

			if (JS::ToInt64(cx, jsValue, &val))
			{
				return val;
			}

			throw smp::JsException();
		}

		template <>
		uint64_t ToSimpleValue(JSContext* cx, const JS::HandleValue& jsValue)
		{
			uint64_t val{};

			if (JS::ToUint64(cx, jsValue, &val))
			{
				return val;
			}

			throw smp::JsException();
		}

		template <>
		float ToSimpleValue(JSContext* cx, const JS::HandleValue& jsValue)
		{
			double val{};

			if (JS::ToNumber(cx, jsValue, &val))
			{
				return static_cast<float>(val);
			}

			throw smp::JsException();
		}

		template <>
		double ToSimpleValue(JSContext* cx, const JS::HandleValue& jsValue)
		{
			double val{};

			if (JS::ToNumber(cx, jsValue, &val))
			{
				return val;
			}

			throw smp::JsException();
		}

		template <>
		std::string ToSimpleValue(JSContext* cx, const JS::HandleValue& jsValue)
		{
			JS::RootedString jsString(cx, JS::ToString(cx, jsValue));
			return ToValue<std::string>(cx, jsString);
		}

		template <>
		std::wstring ToSimpleValue(JSContext* cx, const JS::HandleValue& jsValue)
		{
			JS::RootedString jsString(cx, JS::ToString(cx, jsValue));
			return ToValue<std::wstring>(cx, jsString);
		}

		template <>
		pfc::string8 ToSimpleValue(JSContext* cx, const JS::HandleValue& jsValue)
		{
			JS::RootedString jsString(cx, JS::ToString(cx, jsValue));
			return ToValue<pfc::string8>(cx, jsString);
		}

		template <>
		std::nullptr_t ToSimpleValue(JSContext*, const JS::HandleValue&)
		{
			return nullptr;
		}
	}

	template <>
	std::string ToValue(JSContext* cx, const JS::HandleString& jsString)
	{
		return qwr::ToU8(ToValue<std::wstring>(cx, jsString));
	}

	template <>
	std::wstring ToValue(JSContext* cx, const JS::HandleString& jsString)
	{
		std::wstring wStr(JS_GetStringLength(jsString), '\0');
		mozilla::Range<char16_t> wCharStr(reinterpret_cast<char16_t*>(wStr.data()), wStr.size());

		if (JS_CopyStringChars(cx, wCharStr, jsString))
		{
			return wStr;
		}

		throw smp::JsException();
	}

	template <>
	pfc::string8 ToValue(JSContext* cx, const JS::HandleString& jsString)
	{
		const auto str = ToValue<std::string>(cx, jsString);
		return str.c_str();
	}
}
