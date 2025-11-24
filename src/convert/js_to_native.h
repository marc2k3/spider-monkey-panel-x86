#pragma once
#include <js_utils/js_object_helper.h>

namespace mozjs::convert::to_native
{
	namespace internal
	{
		template <class T>
		inline constexpr bool IsJsSimpleConvertableImplV = std::disjunction_v<std::is_fundamental<T>, std::is_same<std::string, T>, std::is_same<std::wstring, T>, std::is_same<pfc::string8, T>>;

		template <class T>
		inline constexpr bool IsJsSimpleConvertableV = IsJsSimpleConvertableImplV<std::remove_cv_t<T>>;

		template <typename T>
		T ToSimpleValue(JSContext* cx, const JS::HandleObject& jsObject)
		{
			auto pNative = mozjs::GetInnerInstancePrivate<std::remove_pointer_t<T>>(cx, jsObject);
			QwrException::ExpectTrue(pNative, "Object is not of valid type");

			return pNative;
		}

		template <typename T>
		T ToSimpleValue(JSContext* cx, const JS::HandleValue& jsValue)
		{
			static_assert(qwr::always_false_v<T>, "Unsupported type");
		}

		template <>
		bool ToSimpleValue<bool>(JSContext* cx, const JS::HandleValue& jsValue);

		template <>
		int8_t ToSimpleValue<int8_t>(JSContext* cx, const JS::HandleValue& jsValue);

		template <>
		int32_t ToSimpleValue<int32_t>(JSContext* cx, const JS::HandleValue& jsValue);

		template <>
		uint8_t ToSimpleValue<uint8_t>(JSContext* cx, const JS::HandleValue& jsValue);

		template <>
		uint32_t ToSimpleValue<uint32_t>(JSContext* cx, const JS::HandleValue& jsValue);

		/// @details Returns only approximate int64_t value, use with care!
		template <>
		int64_t ToSimpleValue<int64_t>(JSContext* cx, const JS::HandleValue& jsValue);

		/// @details Returns only approximate uint64_t value, use with care!
		template <>
		uint64_t ToSimpleValue<uint64_t>(JSContext* cx, const JS::HandleValue& jsValue);

		template <>
		float ToSimpleValue<float>(JSContext* cx, const JS::HandleValue& jsValue);

		template <>
		double ToSimpleValue<double>(JSContext* cx, const JS::HandleValue& jsValue);

		template <>
		std::string ToSimpleValue<std::string>(JSContext* cx, const JS::HandleValue& jsValue);

		template <>
		std::wstring ToSimpleValue<std::wstring>(JSContext* cx, const JS::HandleValue& jsValue);

		template <>
		pfc::string8 ToSimpleValue<pfc::string8>(JSContext* cx, const JS::HandleValue& jsValue);

		template <>
		std::nullptr_t ToSimpleValue<std::nullptr_t>(JSContext* cx, const JS::HandleValue& jsValue);

		template <typename T>
		std::optional<T> ToOptional(JSContext* cx, const JS::HandleValue& jsValue)
		{
			if (jsValue.isNullOrUndefined())
			{
				return std::nullopt;
			}

			return ToSimpleValue<T>(cx, jsValue);
		}

		template <typename T>
		std::vector<T> ToVector(JSContext* cx, JS::HandleObject jsObject)
		{
			std::vector<T> nativeValues;
			ProcessArray<T>(cx, jsObject, [&nativeValues](T&& nativeValue) { nativeValues.push_back(std::forward<T>(nativeValue)); });

			return nativeValues;
		}

		template <typename T>
		std::vector<T> ToVector(JSContext* cx, JS::HandleValue jsValue)
		{
			JS::RootedObject jsObject(cx, jsValue.toObjectOrNull());
			QwrException::ExpectTrue(jsObject, "Value is not a JS object");

			return ToVector<T>(cx, jsObject);
		}

		template <typename T, typename F>
		void ProcessArray(JSContext* cx, JS::HandleObject jsObject, F&& workerFunc)
		{
			bool is{};
			if (!JS::IsArrayObject(cx, jsObject, &is) || !is)
				throw QwrException("Not a valid JS array");
	
			uint32_t arraySize{};
			if (!JS::GetArrayLength(cx, jsObject, &arraySize) || arraySize == 0)
				return;

			JS::RootedValue arrayElement(cx);
			for (const auto i : ranges::views::indices(arraySize))
			{
				const bool ok = JS_GetElement(cx, jsObject, i, &arrayElement);
				QwrException::ExpectTrue(ok, "Internal error: JS_GetElement failed");

				if constexpr (std::is_same_v<T, std::string>)
				{
					QwrException::ExpectTrue(arrayElement.isString(), "arrayElement is not a string");
				}

				workerFunc(ToValue<T>(cx, arrayElement));
			}
		}
	} // namespace internal

	template <typename T>
	T ToValue(JSContext* cx, JS::HandleValue jsValue)
	{
		if constexpr (internal::IsJsSimpleConvertableV<T>)
		{
			return internal::ToSimpleValue<T>(cx, jsValue);
		}
		else if constexpr (qwr::is_specialization_of_v<T, std::optional>)
		{
			return internal::ToOptional<typename T::value_type>(cx, jsValue);
		}
		else if constexpr (std::is_pointer_v<T>)
		{
			if (!jsValue.isObjectOrNull())
			{
				throw QwrException("Value is not a JS object");
			}

			if (jsValue.isNull())
			{
				// Not an error: null might be a valid argument
				return static_cast<T>(nullptr);
			}

			JS::RootedObject jsObject(cx, &jsValue.toObject());
			return internal::ToSimpleValue<T>(cx, jsObject);
		}
		else if constexpr (qwr::is_specialization_of_v<T, std::vector>)
		{
			return internal::ToVector<typename T::value_type>(cx, jsValue);
		}
		else
		{
			static_assert(qwr::always_false_v<T>, "Unsupported type");
		}
	}

	template <typename T>
	T ToValue(JSContext* cx, const JS::HandleString& jsString)
	{
		static_assert(0, "Unsupported type");
	}

	template <>
	std::string ToValue(JSContext* cx, const JS::HandleString& jsString);

	template <>
	std::wstring ToValue(JSContext* cx, const JS::HandleString& jsString);

	template <>
	pfc::string8 ToValue(JSContext* cx, const JS::HandleString& jsString);

	template <typename T, typename F>
	void ProcessArray(JSContext* cx, JS::HandleValue jsValue, F&& workerFunc)
	{
		if (jsValue.isObject())
		{
			auto jsObject = JS::RootedObject(cx, jsValue.toObjectOrNull());
			if (jsObject)
			{
				internal::ProcessArray<T>(cx, jsObject, std::forward<F>(workerFunc));
				return;
			}
		}

		throw QwrException("Not a valid JS array");
	}
}
