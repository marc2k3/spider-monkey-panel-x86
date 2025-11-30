#include <stdafx.h>
#include "gdi.h"

#include <2K3/FileHelper.hpp>
#include <2K3/LoadImageAsync.hpp>
#include <interfaces/gdi_bitmap.h>
#include <interfaces/gdi_font.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_hwnd_helpers.h>
#include <js_utils/js_image_helpers.h>
#include <js_utils/js_object_helper.h>
#include <utils/gdi_error_helpers.h>
#include <utils/gdi_helpers.h>

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
	Gdi::FinalizeJsObject,
	nullptr,
	nullptr,
	nullptr,
	nullptr
};

JSClass jsClass = {
	"GdiUtils",
	kDefaultClassFlags,
	&jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE(CreateImage, Gdi::CreateImage)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(Font, Gdi::Font, Gdi::FontWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(Image, Gdi::Image)
MJS_DEFINE_JS_FN_FROM_NATIVE(LoadImageAsync, Gdi::LoadImageAsync)
MJS_DEFINE_JS_FN_FROM_NATIVE(LoadImageAsyncV2, Gdi::LoadImageAsyncV2)

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
	{
		JS_FN("CreateImage", CreateImage, 2, kDefaultPropsFlags),
		JS_FN("Font", Font, 2, kDefaultPropsFlags),
		JS_FN("Image", Image, 1, kDefaultPropsFlags),
		JS_FN("LoadImageAsync", LoadImageAsync, 2, kDefaultPropsFlags),
		JS_FN("LoadImageAsyncV2", LoadImageAsyncV2, 2, kDefaultPropsFlags),
		JS_FS_END,
	});

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
	{
		JS_PS_END,
	});

} // namespace

namespace mozjs
{

const JSClass Gdi::JsClass = jsClass;
const JSFunctionSpec* Gdi::JsFunctions = jsFunctions.data();
const JSPropertySpec* Gdi::JsProperties = jsProperties.data();

Gdi::Gdi(JSContext* ctx) : m_ctx(ctx) {}

std::unique_ptr<Gdi> Gdi::CreateNative(JSContext* ctx)
{
	return std::unique_ptr<Gdi>(new Gdi(ctx));
}

size_t Gdi::GetInternalSize()
{
	return 0;
}

JSObject* Gdi::CreateImage(uint32_t w, uint32_t h)
{
	std::unique_ptr<Gdiplus::Bitmap> img(new Gdiplus::Bitmap(w, h, PixelFormat32bppPARGB));
	qwr::CheckGdiPlusObject(img);

	return JsGdiBitmap::CreateJs(m_ctx, std::move(img));
}

JSObject* Gdi::Font(const std::wstring& fontName, uint32_t pxSize, uint32_t style)
{
	return JsGdiFont::Constructor(m_ctx, fontName, pxSize, style);
}

JSObject* Gdi::FontWithOpt(size_t optArgCount, const std::wstring& fontName, uint32_t pxSize, uint32_t style)
{
	switch (optArgCount)
	{
	case 0:
		return Font(fontName, pxSize, style);
	case 1:
		return Font(fontName, pxSize);
	default:
		throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

JSObject* Gdi::Image(const std::wstring& path)
{
	auto bitmap = FileHelper(path).load_image();
	if (!bitmap)
		return nullptr;

	return JsGdiBitmap::CreateJs(m_ctx, std::move(bitmap));
}

std::uint32_t Gdi::LoadImageAsync(uint32_t /*window_id*/, const std::wstring& path)
{
	const auto wnd = GetPanelHwndForCurrentGlobal(m_ctx);
	QwrException::ExpectTrue(wnd, "Method called before fb2k was initialized completely");

	static uint32_t s_task_id{};

	auto task = fb2k::service_new<::LoadImageAsync>(wnd, path, ++s_task_id);
	fb2k::cpuThreadPool::get()->runSingle(task);
	return s_task_id;
}

JSObject* Gdi::LoadImageAsyncV2(uint32_t /*window_id*/, const std::wstring& path)
{
	const auto wnd = GetPanelHwndForCurrentGlobal(m_ctx);
	QwrException::ExpectTrue(wnd, "Method called before fb2k was initialized completely");

	return mozjs::image::GetImagePromise(m_ctx, wnd, path);
}

} // namespace mozjs
