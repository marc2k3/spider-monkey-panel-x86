#pragma once
#include <js_objects/object_base.h>

namespace mozjs
{
	class JsFbMetadbHandle;
	class JsFbMetadbHandleList;

	class JsFbTitleFormat : public JsObjectBase<JsFbTitleFormat>
	{
	public:
		static constexpr bool HasProto = true;
		static constexpr bool HasGlobalProto = true;
		static constexpr bool HasStaticFunctions = false;
		static constexpr bool HasProxy = false;
		static constexpr bool HasPostCreate = false;

		static const JSClass JsClass;
		static const JSFunctionSpec* JsFunctions;
		static const JSPropertySpec* JsProperties;
		static const JsPrototypeId PrototypeId;
		static const JSNative JsConstructor;

	public:
		~JsFbTitleFormat() override = default;

		static std::unique_ptr<JsFbTitleFormat> CreateNative(JSContext* cx, const std::string& expr);
		static size_t GetInternalSize(const std::string& expr);

	public:
		titleformat_object::ptr GetTitleFormat();

	public: // ctor
		static JSObject* Constructor(JSContext* cx, const std::string& expr);

	public:
		pfc::string8 Eval(bool force = false);
		pfc::string8 EvalWithOpt(size_t optArgCount, bool force);
		pfc::string8 EvalWithMetadb(JsFbMetadbHandle* handle);
		JS::Value EvalWithMetadbs(JsFbMetadbHandleList* handles);

	private:
		JsFbTitleFormat(JSContext* cx, const std::string& expr);

	private:
		JSContext* pJsCtx_ = nullptr;
		titleformat_object::ptr titleFormatObject_;
	};
}
