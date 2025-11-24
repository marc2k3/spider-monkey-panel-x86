#include <stdafx.h>
#include "fb_title_format.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

using namespace smp;

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	JsFbTitleFormat::FinalizeJsObject,
	nullptr,
	nullptr,
	nullptr,
	nullptr
};

JSClass jsClass = {
	"FbTitleFormat",
	kDefaultClassFlags,
	&jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(Eval, JsFbTitleFormat::Eval, JsFbTitleFormat::EvalWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(EvalWithMetadb, JsFbTitleFormat::EvalWithMetadb)
MJS_DEFINE_JS_FN_FROM_NATIVE(EvalWithMetadbs, JsFbTitleFormat::EvalWithMetadbs)

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
	{
		JS_FN("Eval", Eval, 0, kDefaultPropsFlags),
		JS_FN("EvalWithMetadb", EvalWithMetadb, 1, kDefaultPropsFlags),
		JS_FN("EvalWithMetadbs", EvalWithMetadbs, 1, kDefaultPropsFlags),
		JS_FS_END,
	});

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
	{
		JS_PS_END,
	});

MJS_DEFINE_JS_FN_FROM_NATIVE(FbTitleFormat_Constructor, JsFbTitleFormat::Constructor)

} // namespace

namespace mozjs
{

const JSClass JsFbTitleFormat::JsClass = jsClass;
const JSFunctionSpec* JsFbTitleFormat::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbTitleFormat::JsProperties = jsProperties.data();
const JsPrototypeId JsFbTitleFormat::PrototypeId = JsPrototypeId::FbTitleFormat;
const JSNative JsFbTitleFormat::JsConstructor = ::FbTitleFormat_Constructor;

JsFbTitleFormat::JsFbTitleFormat(JSContext* cx, const std::string& expr)
	: pJsCtx_(cx)
{
	titleformat_compiler::get()->compile_safe(titleFormatObject_, expr.c_str());
}

std::unique_ptr<JsFbTitleFormat>
JsFbTitleFormat::CreateNative(JSContext* cx, const std::string& expr)
{
	return std::unique_ptr<JsFbTitleFormat>(new JsFbTitleFormat(cx, expr));
}

size_t JsFbTitleFormat::GetInternalSize(const std::string& /*expr*/)
{
	return sizeof(titleformat_object);
}

titleformat_object::ptr JsFbTitleFormat::GetTitleFormat()
{
	return titleFormatObject_;
}

JSObject* JsFbTitleFormat::Constructor(JSContext* cx, const std::string& expr)
{
	return JsFbTitleFormat::CreateJs(cx, expr);
}

pfc::string8 JsFbTitleFormat::Eval(bool force)
{
	pfc::string8 text;

	if (playback_control::get()->playback_format_title(nullptr, text, titleFormatObject_, nullptr, playback_control::display_level_all))
	{
		return text;
	}
	else if (force)
	{
		metadb_handle_ptr handle;

		if (!metadb::g_get_random_handle(handle))
		{
			metadb::get()->handle_create(handle, make_playable_location("file://C:\\________.ogg", 0));
		}

		handle->format_title(nullptr, text, titleFormatObject_, nullptr);
	}

	return text;
}

pfc::string8 JsFbTitleFormat::EvalWithOpt(size_t optArgCount, bool force)
{
	switch (optArgCount)
	{
	case 0:
		return Eval(force);
	case 1:
		return Eval();
	default:
		throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

pfc::string8 JsFbTitleFormat::EvalWithMetadb(JsFbMetadbHandle* handle)
{
	QwrException::ExpectTrue(handle, "handle argument is null");

	pfc::string8 text;
	handle->GetHandle()->format_title(nullptr, text, titleFormatObject_, nullptr);
	return text;
}

JS::Value JsFbTitleFormat::EvalWithMetadbs(JsFbMetadbHandleList* handles)
{
	QwrException::ExpectTrue(handles, "handles argument is null");

	const auto& native = handles->GetHandleList();
	const auto count = native.get_count();
	pfc::array_t<pfc::string8> values;
	values.set_size(count);

	auto api = metadb_v2::get();

	api->queryMultiParallel_(native, [&](size_t index, const metadb_v2::rec_t& rec)
		{
			api->formatTitle_v2(native[index], rec, nullptr, values[index], titleFormatObject_, nullptr);
		});

	JS::RootedValue jsValue(pJsCtx_);
	convert::to_js::ToArrayValue(pJsCtx_, values, &jsValue);
	return jsValue;
}

} // namespace mozjs
