#pragma once

namespace mozjs
{
	class JsFbPlayingItemLocation : public JsObjectBase<JsFbPlayingItemLocation>
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
		~JsFbPlayingItemLocation() override = default;

		static std::unique_ptr<JsFbPlayingItemLocation> CreateNative(JSContext* cx, bool is_valid, uint32_t playlistIndex, uint32_t playlistItemIndex);
		static size_t GetInternalSize(bool is_valid, uint32_t playlistIndex, uint32_t playlistItemIndex);

	public:
		bool get_IsValid();
		int32_t get_PlaylistIndex();
		int32_t get_PlaylistItemIndex();

	private:
		JsFbPlayingItemLocation(JSContext* cx, bool is_valid, uint32_t playlistIndex, uint32_t playlistItemIndex);

		JSContext* m_ctx{};
		bool m_is_valid{};
		int32_t m_playlistIndex = -1;
		int32_t m_playlistItemIndex = -1;
	};
}
