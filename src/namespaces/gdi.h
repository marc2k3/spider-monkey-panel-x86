#pragma once

namespace mozjs
{
	class Gdi : public JsObjectBase<Gdi>
	{
	public:
		static constexpr bool HasProto = false;
		static constexpr bool HasProxy = false;
		static constexpr bool HasPostCreate = false;

		static const JSClass JsClass;
		static const JSFunctionSpec* JsFunctions;
		static const JSPropertySpec* JsProperties;

	public:
		~Gdi() override = default;

		static std::unique_ptr<Gdi> CreateNative(JSContext* ctx);
		static size_t GetInternalSize();

	public:
		JSObject* CreateImage(uint32_t w, uint32_t h);
		JSObject* Font(const std::wstring& fontName, uint32_t pxSize, uint32_t style = 0);
		JSObject* FontWithOpt(size_t optArgCount, const std::wstring& fontName, uint32_t pxSize, uint32_t style);
		JSObject* Image(const std::wstring& path);
		std::uint32_t LoadImageAsync(uint32_t /*window_id*/, const std::wstring& path);
		JSObject* LoadImageAsyncV2(uint32_t /*window_id*/, const std::wstring& path);

	private:
		Gdi(JSContext* ctx);

	private:
		JSContext* m_ctx{};
	};
}
