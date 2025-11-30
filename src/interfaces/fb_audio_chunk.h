#pragma once
#include <js_objects/object_base.h>

namespace mozjs
{
	class JsFbAudioChunk : public JsObjectBase<JsFbAudioChunk>
	{
	public:
		static constexpr bool HasProto = true;
		static constexpr bool HasGlobalProto = false;
		static constexpr bool HasProxy = false;
		static constexpr bool HasPostCreate = false;

		static const JSClass JsClass;
		static const JSFunctionSpec* JsFunctions;
		static const JSPropertySpec* JsProperties;
		static const JsPrototypeId PrototypeId;

	public:
		~JsFbAudioChunk() override = default;

		static std::unique_ptr<JsFbAudioChunk> CreateNative(JSContext* cx, const audio_chunk_impl& chunk);
		static size_t GetInternalSize(const audio_chunk_impl& chunk);

	public:
		JS::Value get_Data();
		uint32_t get_ChannelConfig();
		uint32_t get_ChannelCount();
		uint32_t get_SampleRate();
		size_t get_SampleCount();

	private:
		JsFbAudioChunk(JSContext* cx, const audio_chunk_impl& chunk);

	private:
		[[maybe_unused]] JSContext* pJsCtx_ = nullptr;
		audio_chunk_impl chunk_;
	};
}
