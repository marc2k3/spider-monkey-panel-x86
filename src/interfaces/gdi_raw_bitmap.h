#pragma once
#include <utils/gdi_helpers.h>

namespace mozjs
{
	class JsGdiRawBitmap : public JsObjectBase<JsGdiRawBitmap>
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
		~JsGdiRawBitmap() override = default;

		static std::unique_ptr<JsGdiRawBitmap> CreateNative(JSContext* cx, Gdiplus::Bitmap* pBmp);
		static size_t GetInternalSize(Gdiplus::Bitmap* pBmp);

	public:
		[[nodiscard]] __notnull
			HDC
			GetHDC() const;

	public: // props
		std::uint32_t get_Height();
		std::uint32_t get_Width();

	private:
		JsGdiRawBitmap(JSContext* cx, wil::unique_hbitmap hBmp, uint32_t width, uint32_t height);

		static wil::unique_hbitmap CreateHBitmapFromGdiPlusBitmap(Gdiplus::Bitmap& bitmap);

		[[maybe_unused]] JSContext* pJsCtx_ = nullptr;
		const wil::unique_hdc pDc_;
		const wil::unique_hbitmap hBmp_;
		const wil::unique_select_object autoBmp_;
		uint32_t width_;
		uint32_t height_;
	};
}
