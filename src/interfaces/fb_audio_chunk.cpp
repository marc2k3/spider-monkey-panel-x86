#include <stdafx.h>
#include "fb_audio_chunk.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

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
	JsFbAudioChunk::FinalizeJsObject,
	nullptr,
	nullptr,
	nullptr,
	nullptr
};

JSClass jsClass = {
	"FbAudioChunk",
	kDefaultClassFlags,
	&jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE(get_Data, JsFbAudioChunk::get_Data);
MJS_DEFINE_JS_FN_FROM_NATIVE(get_ChannelConfig, JsFbAudioChunk::get_ChannelConfig);
MJS_DEFINE_JS_FN_FROM_NATIVE(get_ChannelCount, JsFbAudioChunk::get_ChannelCount);
MJS_DEFINE_JS_FN_FROM_NATIVE(get_SampleCount, JsFbAudioChunk::get_SampleCount);
MJS_DEFINE_JS_FN_FROM_NATIVE(get_SampleRate, JsFbAudioChunk::get_SampleRate);

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
	{
		JS_PSG("Data", get_Data, kDefaultPropsFlags),
		JS_PSG("ChannelConfig", get_ChannelConfig, kDefaultPropsFlags),
		JS_PSG("ChannelCount", get_ChannelCount, kDefaultPropsFlags),
		JS_PSG("SampleCount", get_SampleCount, kDefaultPropsFlags),
		JS_PSG("SampleRate", get_SampleRate, kDefaultPropsFlags),
		JS_PS_END,
	});

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
	{
		JS_FS_END,
	});

} // namespace

namespace mozjs
{

const JSClass JsFbAudioChunk::JsClass = jsClass;
const JSFunctionSpec* JsFbAudioChunk::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbAudioChunk::JsProperties = jsProperties.data();
const JsPrototypeId JsFbAudioChunk::PrototypeId = JsPrototypeId::FbAudioChunk;

JsFbAudioChunk::JsFbAudioChunk(JSContext* cx, const audio_chunk_impl& chunk) : pJsCtx_(cx), chunk_(chunk) {}

std::unique_ptr<JsFbAudioChunk> JsFbAudioChunk::CreateNative(JSContext* cx, const audio_chunk_impl& chunk)
{
	return std::unique_ptr<JsFbAudioChunk>(new JsFbAudioChunk(cx, chunk));
}

size_t JsFbAudioChunk::GetInternalSize(const audio_chunk_impl&)
{
	return sizeof(audio_chunk_impl);
}

JS::Value JsFbAudioChunk::get_Data()
{
	JS::RootedValue jsValue(pJsCtx_);
	convert::to_js::ToArrayValue(pJsCtx_, std::span(chunk_.get_data(), chunk_.get_used_size()), &jsValue);
	return jsValue;
}

uint32_t JsFbAudioChunk::get_ChannelConfig()
{
	return chunk_.get_channel_config();
}

uint32_t JsFbAudioChunk::get_ChannelCount()
{
	return chunk_.get_channel_count();
}

uint32_t JsFbAudioChunk::get_SampleRate()
{
	return chunk_.get_sample_rate();
}

size_t JsFbAudioChunk::get_SampleCount()
{
	return chunk_.get_sample_count();
}

} // namespace mozjs
