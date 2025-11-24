#include <stdafx.h>
#include "gdi_graphics.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_objects/gdi_bitmap.h>
#include <js_objects/gdi_font.h>
#include <js_objects/gdi_raw_bitmap.h>
#include <js_objects/measure_string_info.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <utils/colour_helpers.h>
#include <utils/gdi_error_helpers.h>
#include <utils/text_helpers.h>

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
	JsGdiGraphics::FinalizeJsObject,
	nullptr,
	nullptr,
	nullptr,
	nullptr
};

JSClass jsClass = {
	"GdiGraphics",
	kDefaultClassFlags,
	&jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE(CalcTextHeight, JsGdiGraphics::CalcTextHeight)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(CalcTextWidth, JsGdiGraphics::CalcTextWidth, JsGdiGraphics::CalcTextWidthWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(DrawEllipse, JsGdiGraphics::DrawEllipse)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(DrawImage, JsGdiGraphics::DrawImage, JsGdiGraphics::DrawImageWithOpt, 2)
MJS_DEFINE_JS_FN_FROM_NATIVE(DrawLine, JsGdiGraphics::DrawLine)
MJS_DEFINE_JS_FN_FROM_NATIVE(DrawPolygon, JsGdiGraphics::DrawPolygon)
MJS_DEFINE_JS_FN_FROM_NATIVE(DrawRect, JsGdiGraphics::DrawRect)
MJS_DEFINE_JS_FN_FROM_NATIVE(DrawRoundRect, JsGdiGraphics::DrawRoundRect)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(DrawString, JsGdiGraphics::DrawString, JsGdiGraphics::DrawStringWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(EstimateLineWrap, JsGdiGraphics::EstimateLineWrap)
MJS_DEFINE_JS_FN_FROM_NATIVE(FillEllipse, JsGdiGraphics::FillEllipse)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(FillGradRect, JsGdiGraphics::FillGradRect, JsGdiGraphics::FillGradRectWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(FillPolygon, JsGdiGraphics::FillPolygon)
MJS_DEFINE_JS_FN_FROM_NATIVE(FillRoundRect, JsGdiGraphics::FillRoundRect)
MJS_DEFINE_JS_FN_FROM_NATIVE(FillSolidRect, JsGdiGraphics::FillSolidRect)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(GdiAlphaBlend, JsGdiGraphics::GdiAlphaBlend, JsGdiGraphics::GdiAlphaBlendWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(GdiDrawBitmap, JsGdiGraphics::GdiDrawBitmap)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(GdiDrawText, JsGdiGraphics::GdiDrawText, JsGdiGraphics::GdiDrawTextWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(MeasureString, JsGdiGraphics::MeasureString, JsGdiGraphics::MeasureStringWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(SetInterpolationMode, JsGdiGraphics::SetInterpolationMode, JsGdiGraphics::SetInterpolationModeWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(SetSmoothingMode, JsGdiGraphics::SetSmoothingMode, JsGdiGraphics::SetSmoothingModeWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(SetTextRenderingHint, JsGdiGraphics::SetTextRenderingHint, JsGdiGraphics::SetTextRenderingHintWithOpt, 1)

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
	{
		JS_FN("CalcTextHeight", CalcTextHeight, 2, kDefaultPropsFlags),
		JS_FN("CalcTextWidth", CalcTextWidth, 2, kDefaultPropsFlags),
		JS_FN("DrawEllipse", DrawEllipse, 6, kDefaultPropsFlags),
		JS_FN("DrawImage", DrawImage, 9, kDefaultPropsFlags),
		JS_FN("DrawLine", DrawLine, 6, kDefaultPropsFlags),
		JS_FN("DrawPolygon", DrawPolygon, 3, kDefaultPropsFlags),
		JS_FN("DrawRect", DrawRect, 6, kDefaultPropsFlags),
		JS_FN("DrawRoundRect", DrawRoundRect, 8, kDefaultPropsFlags),
		JS_FN("DrawString", DrawString, 7, kDefaultPropsFlags),
		JS_FN("EstimateLineWrap", EstimateLineWrap, 3, kDefaultPropsFlags),
		JS_FN("FillEllipse", FillEllipse, 5, kDefaultPropsFlags),
		JS_FN("FillGradRect", FillGradRect, 7, kDefaultPropsFlags),
		JS_FN("FillPolygon", FillPolygon, 3, kDefaultPropsFlags),
		JS_FN("FillRoundRect", FillRoundRect, 7, kDefaultPropsFlags),
		JS_FN("FillSolidRect", FillSolidRect, 5, kDefaultPropsFlags),
		JS_FN("GdiAlphaBlend", GdiAlphaBlend, 9, kDefaultPropsFlags),
		JS_FN("GdiDrawBitmap", GdiDrawBitmap, 9, kDefaultPropsFlags),
		JS_FN("GdiDrawText", GdiDrawText, 7, kDefaultPropsFlags),
		JS_FN("MeasureString", MeasureString, 6, kDefaultPropsFlags),
		JS_FN("SetInterpolationMode", SetInterpolationMode, 0, kDefaultPropsFlags),
		JS_FN("SetSmoothingMode", SetSmoothingMode, 0, kDefaultPropsFlags),
		JS_FN("SetTextRenderingHint", SetTextRenderingHint, 0, kDefaultPropsFlags),
		JS_FS_END,
	});

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
	{
		JS_PS_END,
	});

} // namespace

namespace mozjs
{

const JSClass JsGdiGraphics::JsClass = jsClass;
const JSFunctionSpec* JsGdiGraphics::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsGdiGraphics::JsProperties = jsProperties.data();
const JsPrototypeId JsGdiGraphics::PrototypeId = JsPrototypeId::GdiGraphics;

JsGdiGraphics::JsGdiGraphics(JSContext* cx)
	: pJsCtx_(cx)
{
}

std::unique_ptr<JsGdiGraphics>
JsGdiGraphics::CreateNative(JSContext* cx)
{
	return std::unique_ptr<JsGdiGraphics>(new JsGdiGraphics(cx));
}

size_t JsGdiGraphics::GetInternalSize()
{
	return 0;
}

Gdiplus::Graphics* JsGdiGraphics::GetGraphicsObject() const
{
	return pGdi_;
}

void JsGdiGraphics::SetGraphicsObject(Gdiplus::Graphics* graphics)
{
	pGdi_ = graphics;
}

uint32_t JsGdiGraphics::CalcTextHeight(const std::wstring& str, JsGdiFont* font)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");
	QwrException::ExpectTrue(font, "font argument is null");

	const auto hDc = pGdi_->GetHDC();
	auto autoHdcReleaser = wil::scope_exit([hDc, pGdi = pGdi_] { pGdi->ReleaseHDC(hDc); });
	auto _ = wil::SelectObject(hDc, font->GetHFont());

	return smp::utils::GetTextHeight(hDc, str);
}

uint32_t JsGdiGraphics::CalcTextWidth(const std::wstring& str, JsGdiFont* font, boolean use_exact)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");
	QwrException::ExpectTrue(font, "font argument is null");

	const auto hDc = pGdi_->GetHDC();
	auto autoHdcReleaser = wil::scope_exit([hDc, pGdi = pGdi_] { pGdi->ReleaseHDC(hDc); });
	auto _ = wil::SelectObject(hDc, font->GetHFont());

	return smp::utils::GetTextWidth(hDc, str, use_exact);
}

uint32_t JsGdiGraphics::CalcTextWidthWithOpt(size_t optArgCount, const std::wstring& str, JsGdiFont* font, boolean use_exact)
{
	switch (optArgCount)
	{
	case 0:
		return CalcTextWidth(str, font, use_exact);
	case 1:
		return CalcTextWidth(str, font);
	default:
		throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void JsGdiGraphics::DrawEllipse(float x, float y, float w, float h, float line_width, uint32_t colour)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");

	Gdiplus::Pen pen(colour, line_width);
	const auto status = pGdi_->DrawEllipse(&pen, x, y, w, h);
	qwr::CheckGdi(status, "DrawEllipse");
}

void JsGdiGraphics::DrawImage(JsGdiBitmap* image,
	float dstX, float dstY, float dstW, float dstH,
	float srcX, float srcY, float srcW, float srcH,
	float angle, uint8_t alpha)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");
	QwrException::ExpectTrue(image, "image argument is null");

	Gdiplus::Bitmap* img = image->GdiBitmap();
	Gdiplus::Matrix oldMatrix;
	Gdiplus::Status status;

	if (angle != 0.0)
	{
		Gdiplus::Matrix m;
		status = m.RotateAt(angle, Gdiplus::PointF{ dstX + dstW / 2, dstY + dstH / 2 });
		qwr::CheckGdi(status, "RotateAt");

		status = pGdi_->GetTransform(&oldMatrix);
		qwr::CheckGdi(status, "GetTransform");

		status = pGdi_->SetTransform(&m);
		qwr::CheckGdi(status, "SetTransform");
	}

	if (alpha < 255)
	{
		Gdiplus::ImageAttributes ia;
		Gdiplus::ColorMatrix cm{};

		cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = cm.m[4][4] = 1.0f;
		cm.m[3][3] = static_cast<float>(alpha) / 255;

		status = ia.SetColorMatrix(&cm);
		qwr::CheckGdi(status, "SetColorMatrix");

		status = pGdi_->DrawImage(img, Gdiplus::RectF(dstX, dstY, dstW, dstH), srcX, srcY, srcW, srcH, Gdiplus::UnitPixel, &ia);
		qwr::CheckGdi(status, "DrawImage");
	}
	else
	{
		status = pGdi_->DrawImage(img, Gdiplus::RectF(dstX, dstY, dstW, dstH), srcX, srcY, srcW, srcH, Gdiplus::UnitPixel);
		qwr::CheckGdi(status, "DrawImage");
	}

	if (angle != 0.0)
	{
		status = pGdi_->SetTransform(&oldMatrix);
		qwr::CheckGdi(status, "SetTransform");
	}
}

void JsGdiGraphics::DrawImageWithOpt(size_t optArgCount, JsGdiBitmap* image,
	float dstX, float dstY, float dstW, float dstH,
	float srcX, float srcY, float srcW, float srcH, float angle,
	uint8_t alpha)
{
	switch (optArgCount)
	{
	case 0:
		return DrawImage(image, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH, angle, alpha);
	case 1:
		return DrawImage(image, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH, angle);
	case 2:
		return DrawImage(image, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH);
	default:
		throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void JsGdiGraphics::DrawLine(float x1, float y1, float x2, float y2, float line_width, uint32_t colour)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");

	Gdiplus::Pen pen(colour, line_width);
	const auto status = pGdi_->DrawLine(&pen, x1, y1, x2, y2);
	qwr::CheckGdi(status, "DrawLine");
}

void JsGdiGraphics::DrawPolygon(uint32_t colour, float line_width, JS::HandleValue points)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");

	std::vector<Gdiplus::PointF> gdiPoints;
	ParsePoints(points, gdiPoints);

	Gdiplus::Pen pen(colour, line_width);
	const auto status = pGdi_->DrawPolygon(&pen, gdiPoints.data(), gdiPoints.size());
	qwr::CheckGdi(status, "DrawPolygon");
}

void JsGdiGraphics::DrawRect(float x, float y, float w, float h, float line_width, uint32_t colour)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");

	Gdiplus::Pen pen(colour, line_width);
	const auto status = pGdi_->DrawRectangle(&pen, x, y, w, h);
	qwr::CheckGdi(status, "DrawRectangle");
}

void JsGdiGraphics::DrawRoundRect(float x, float y, float w, float h, float arc_width, float arc_height, float line_width, uint32_t colour)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");
	QwrException::ExpectTrue(2 * arc_width <= w && 2 * arc_height <= h, "Arc argument has invalid value");

	Gdiplus::Pen pen(colour, line_width);
	Gdiplus::GraphicsPath gp;
	GetRoundRectPath(gp, Gdiplus::RectF{ x, y, w, h }, arc_width, arc_height);

	auto status = pen.SetStartCap(Gdiplus::LineCapRound);
	qwr::CheckGdi(status, "SetStartCap");

	status = pen.SetEndCap(Gdiplus::LineCapRound);
	qwr::CheckGdi(status, "SetEndCap");

	status = pGdi_->DrawPath(&pen, &gp);
	qwr::CheckGdi(status, "DrawPath");
}

void JsGdiGraphics::DrawString(const std::wstring& str, JsGdiFont* font, uint32_t colour, float x, float y, float w, float h, uint32_t flags)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");
	QwrException::ExpectTrue(font, "font argument is null");

	Gdiplus::Font* pGdiFont = font->GdiFont();
	QwrException::ExpectTrue(pGdiFont, "Internal error: GdiFont is null");

	Gdiplus::SolidBrush br(colour);
	Gdiplus::StringFormat fmt(Gdiplus::StringFormat::GenericTypographic());
	Gdiplus::Status status{};

	if (flags != 0)
	{
		status = fmt.SetAlignment(static_cast<Gdiplus::StringAlignment>((flags >> 28) & 0x3)); //0xf0000000
		qwr::CheckGdi(status, "SetAlignment");

		status = fmt.SetLineAlignment(static_cast<Gdiplus::StringAlignment>((flags >> 24) & 0x3)); //0x0f000000
		qwr::CheckGdi(status, "SetLineAlignment");

		status = fmt.SetTrimming(static_cast<Gdiplus::StringTrimming>((flags >> 20) & 0x7)); //0x00f00000
		qwr::CheckGdi(status, "SetTrimming");

		status = fmt.SetFormatFlags(static_cast<Gdiplus::StringAlignment>(flags & 0x7FFF)); //0x0000ffff
		qwr::CheckGdi(status, "SetFormatFlags");
	}

	status = pGdi_->DrawString(str.c_str(), -1, pGdiFont, Gdiplus::RectF(x, y, w, h), &fmt, &br);
	qwr::CheckGdi(status, "DrawString");
}

void JsGdiGraphics::DrawStringWithOpt(size_t optArgCount, const std::wstring& str, JsGdiFont* font, uint32_t colour,
	float x, float y, float w, float h,
	uint32_t flags)
{
	switch (optArgCount)
	{
	case 0:
		return DrawString(str, font, colour, x, y, w, h, flags);
	case 1:
		return DrawString(str, font, colour, x, y, w, h);
	default:
		throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

JSObject* JsGdiGraphics::EstimateLineWrap(const std::wstring& str, JsGdiFont* font, uint32_t max_width)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");
	QwrException::ExpectTrue(font, "font argument is null");

	std::vector<smp::utils::WrappedTextLine> result;
	{
		const auto hDc = pGdi_->GetHDC();
		auto autoHdcReleaser = wil::scope_exit([hDc, pGdi = pGdi_] { pGdi->ReleaseHDC(hDc); });
		auto _ = wil::SelectObject(hDc, font->GetHFont());

		result = smp::utils::WrapText(hDc, str, max_width);
	}

	JS::RootedObject jsArray(pJsCtx_, JS::NewArrayObject(pJsCtx_, result.size() * 2));
	JsException::ExpectTrue(jsArray);

	JS::RootedValue jsValue(pJsCtx_);
	size_t i = 0;
	for (const auto& [text, width]: result)
	{
		convert::to_js::ToValue(pJsCtx_, text, &jsValue);

		if (!JS_SetElement(pJsCtx_, jsArray, i++, jsValue))
		{
			throw JsException();
		}

		jsValue.setNumber((uint32_t)width);
		if (!JS_SetElement(pJsCtx_, jsArray, i++, jsValue))
		{
			throw JsException();
		}
	}

	return jsArray;
}

void JsGdiGraphics::FillEllipse(float x, float y, float w, float h, uint32_t colour)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");

	Gdiplus::SolidBrush br(colour);
	const auto status = pGdi_->FillEllipse(&br, x, y, w, h);
	qwr::CheckGdi(status, "FillEllipse");
}

void JsGdiGraphics::FillGradRect(float x, float y, float w, float h, float angle, uint32_t colour1, uint32_t colour2, float focus)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");

	const Gdiplus::RectF rect{ x, y, w, h };
	Gdiplus::LinearGradientBrush brush(rect, colour1, colour2, angle, TRUE);
	auto status = brush.SetBlendTriangularShape(focus);
	qwr::CheckGdi(status, "SetBlendTriangularShape");

	status = pGdi_->FillRectangle(&brush, rect);
	qwr::CheckGdi(status, "FillRectangle");
}

void JsGdiGraphics::FillGradRectWithOpt(size_t optArgCount, float x, float y, float w, float h, float angle, uint32_t colour1, uint32_t colour2, float focus)
{
	switch (optArgCount)
	{
	case 0:
		return FillGradRect(x, y, w, h, angle, colour1, colour2, focus);
	case 1:
		return FillGradRect(x, y, w, h, angle, colour1, colour2);
	default:
		throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void JsGdiGraphics::FillPolygon(uint32_t colour, uint32_t fillmode, JS::HandleValue points)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");

	std::vector<Gdiplus::PointF> gdiPoints;
	ParsePoints(points, gdiPoints);

	Gdiplus::SolidBrush br(colour);
	const auto status = pGdi_->FillPolygon(&br, gdiPoints.data(), gdiPoints.size(), static_cast<Gdiplus::FillMode>(fillmode));
	qwr::CheckGdi(status, "FillPolygon");
}

void JsGdiGraphics::FillRoundRect(float x, float y, float w, float h, float arc_width, float arc_height, uint32_t colour)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");

	QwrException::ExpectTrue(2 * arc_width <= w && 2 * arc_height <= h, "Arc argument has invalid value");

	Gdiplus::SolidBrush br(colour);
	Gdiplus::GraphicsPath gp;
	const Gdiplus::RectF rect{ x, y, w, h };
	GetRoundRectPath(gp, rect, arc_width, arc_height);

	const auto status = pGdi_->FillPath(&br, &gp);
	qwr::CheckGdi(status, "FillPath");
}

void JsGdiGraphics::FillSolidRect(float x, float y, float w, float h, uint32_t colour)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");

	Gdiplus::SolidBrush brush(colour);
	const auto status = pGdi_->FillRectangle(&brush, x, y, w, h);
	qwr::CheckGdi(status, "FillRectangle");
}

void JsGdiGraphics::GdiAlphaBlend(JsGdiRawBitmap* bitmap,
	int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
	int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH,
	uint8_t alpha)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");
	QwrException::ExpectTrue(bitmap, "bitmap argument is null");

	const auto srcDc = bitmap->GetHDC();
	assert(srcDc);

	const auto hDc = pGdi_->GetHDC();
	auto autoHdcReleaser = wil::scope_exit([pGdi = pGdi_, hDc]() { pGdi->ReleaseHDC(hDc); });

	BOOL bRet = ::GdiAlphaBlend(hDc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, srcW, srcH, BLENDFUNCTION{ AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA });
	qwr::CheckWinApi(bRet, "GdiAlphaBlend");
}

void JsGdiGraphics::GdiAlphaBlendWithOpt(size_t optArgCount, JsGdiRawBitmap* bitmap,
	int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
	int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH,
	uint8_t alpha)
{
	switch (optArgCount)
	{
	case 0:
		return GdiAlphaBlend(bitmap, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH, alpha);
	case 1:
		return GdiAlphaBlend(bitmap, dstX, dstY, dstW, dstH, srcX, srcY, srcW, srcH);
	default:
		throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void JsGdiGraphics::GdiDrawBitmap(JsGdiRawBitmap* bitmap,
	int32_t dstX, int32_t dstY, uint32_t dstW, uint32_t dstH,
	int32_t srcX, int32_t srcY, uint32_t srcW, uint32_t srcH)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");
	QwrException::ExpectTrue(bitmap, "bitmap argument is null");

	HDC srcDc = bitmap->GetHDC();
	assert(srcDc);

	HDC hDc = pGdi_->GetHDC();
	auto autoHdcReleaser = wil::scope_exit([pGdi = pGdi_, hDc]() { pGdi->ReleaseHDC(hDc); });

	BOOL bRet;
	if (dstW == srcW && dstH == srcH)
	{
		bRet = BitBlt(hDc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, SRCCOPY);
		qwr::CheckWinApi(bRet, "BitBlt");
	}
	else
	{
		bRet = SetStretchBltMode(hDc, HALFTONE);
		qwr::CheckWinApi(bRet, "SetStretchBltMode");

		bRet = SetBrushOrgEx(hDc, 0, 0, nullptr);
		qwr::CheckWinApi(bRet, "SetBrushOrgEx");

		bRet = StretchBlt(hDc, dstX, dstY, dstW, dstH, srcDc, srcX, srcY, srcW, srcH, SRCCOPY);
		qwr::CheckWinApi(bRet, "StretchBlt");
	}
}

void JsGdiGraphics::GdiDrawText(const std::wstring& str, JsGdiFont* font, uint32_t colour, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t format)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");
	QwrException::ExpectTrue(font, "font argument is null");

	const auto hDc = pGdi_->GetHDC();
	auto autoHdcReleaser = wil::scope_exit([pGdi = pGdi_, hDc] { pGdi->ReleaseHDC(hDc); });
	auto _ = wil::SelectObject(hDc, font->GetHFont());

	RECT rc{ x, y, static_cast<LONG>(x + w), static_cast<LONG>(y + h) };
	DRAWTEXTPARAMS dpt = { sizeof(DRAWTEXTPARAMS), 4, 0, 0, 0 };

	SetTextColor(hDc, smp::colour::ArgbToColorref(colour));

	int iRet = SetBkMode(hDc, TRANSPARENT);
	qwr::CheckWinApi(CLR_INVALID != iRet, "SetBkMode");

	UINT uRet = SetTextAlign(hDc, TA_LEFT | TA_TOP | TA_NOUPDATECP);
	qwr::CheckWinApi(GDI_ERROR != uRet, "SetTextAlign");

	if (format & DT_MODIFYSTRING)
	{
		format &= ~DT_MODIFYSTRING;
	}

	// Well, magic :P
	if (format & DT_CALCRECT)
	{
		const RECT rc_old = rc;

		RECT rc_calc = rc;
		iRet = DrawText(hDc, str.c_str(), -1, &rc_calc, format);
		qwr::CheckWinApi(iRet, "DrawText");

		format &= ~DT_CALCRECT;

		// adjust vertical align
		if (format & DT_VCENTER)
		{
			rc.top = rc_old.top + (((rc_old.bottom - rc_old.top) - (rc_calc.bottom - rc_calc.top)) >> 1);
			rc.bottom = rc.top + (rc_calc.bottom - rc_calc.top);
		}
		else if (format & DT_BOTTOM)
		{
			rc.top = rc_old.bottom - (rc_calc.bottom - rc_calc.top);
		}
	}

	iRet = DrawTextEx(hDc, const_cast<wchar_t*>(str.c_str()), -1, &rc, format, &dpt);
	qwr::CheckWinApi(iRet, "DrawTextEx");
}

void JsGdiGraphics::GdiDrawTextWithOpt(size_t optArgCount, const std::wstring& str, JsGdiFont* font, uint32_t colour,
	int32_t x, int32_t y, uint32_t w, uint32_t h,
	uint32_t format)
{
	switch (optArgCount)
	{
	case 0:
		return GdiDrawText(str, font, colour, x, y, w, h, format);
	case 1:
		return GdiDrawText(str, font, colour, x, y, w, h);
	default:
		throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

JSObject* JsGdiGraphics::MeasureString(const std::wstring& str, JsGdiFont* font, float x, float y, float w, float h, uint32_t flags)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");
	QwrException::ExpectTrue(font, "font argument is null");

	Gdiplus::Font* fn = font->GdiFont();
	assert(fn);

	Gdiplus::StringFormat fmt = Gdiplus::StringFormat::GenericTypographic();
	if (flags != 0)
	{
		fmt.SetAlignment(static_cast<Gdiplus::StringAlignment>((flags >> 28) & 0x3));     //0xf0000000
		fmt.SetLineAlignment(static_cast<Gdiplus::StringAlignment>((flags >> 24) & 0x3)); //0x0f000000
		fmt.SetTrimming(static_cast<Gdiplus::StringTrimming>((flags >> 20) & 0x7));       //0x00f00000
		fmt.SetFormatFlags(static_cast<Gdiplus::StringFormatFlags>(flags & 0x7FFF));        //0x0000ffff
	}

	Gdiplus::RectF bound;
	int chars;
	int lines;
	const auto status = pGdi_->MeasureString(str.c_str(), -1, fn, Gdiplus::RectF(x, y, w, h), &fmt, &bound, &chars, &lines);
	qwr::CheckGdi(status, "MeasureString");

	return JsMeasureStringInfo::CreateJs(pJsCtx_, bound.X, bound.Y, bound.Width, bound.Height, lines, chars);
}

JSObject* JsGdiGraphics::MeasureStringWithOpt(size_t optArgCount, const std::wstring& str, JsGdiFont* font,
	float x, float y, float w, float h,
	uint32_t flags)
{
	switch (optArgCount)
	{
	case 0:
		return MeasureString(str, font, x, y, w, h, flags);
	case 1:
		return MeasureString(str, font, x, y, w, h);
	default:
		throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void JsGdiGraphics::SetInterpolationMode(uint32_t mode)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");

	const auto status = pGdi_->SetInterpolationMode(static_cast<Gdiplus::InterpolationMode>(mode));
	qwr::CheckGdi(status, "SetInterpolationMode");
}

void JsGdiGraphics::SetInterpolationModeWithOpt(size_t optArgCount, uint32_t mode)
{
	switch (optArgCount)
	{
	case 0:
		return SetInterpolationMode(mode);
	case 1:
		return SetInterpolationMode();
	default:
		throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void JsGdiGraphics::SetSmoothingMode(uint32_t mode)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");

	const auto status = pGdi_->SetSmoothingMode(static_cast<Gdiplus::SmoothingMode>(mode));
	qwr::CheckGdi(status, "SetSmoothingMode");
}

void JsGdiGraphics::SetSmoothingModeWithOpt(size_t optArgCount, uint32_t mode)
{
	switch (optArgCount)
	{
	case 0:
		return SetSmoothingMode(mode);
	case 1:
		return SetSmoothingMode();
	default:
		throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void JsGdiGraphics::SetTextRenderingHint(uint32_t mode)
{
	QwrException::ExpectTrue(pGdi_, "Internal error: Gdiplus::Graphics object is null");

	const auto status = pGdi_->SetTextRenderingHint(static_cast<Gdiplus::TextRenderingHint>(mode));
	qwr::CheckGdi(status, "SetTextRenderingHint");
}

void JsGdiGraphics::SetTextRenderingHintWithOpt(size_t optArgCount, uint32_t mode)
{
	switch (optArgCount)
	{
	case 0:
		return SetTextRenderingHint(mode);
	case 1:
		return SetTextRenderingHint();
	default:
		throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void JsGdiGraphics::GetRoundRectPath(Gdiplus::GraphicsPath& gp, const Gdiplus::RectF& rect, float arc_width, float arc_height) const
{
	const float arc_dia_w = arc_width * 2;
	const float arc_dia_h = arc_height * 2;
	Gdiplus::RectF corner{ rect.X, rect.Y, arc_dia_w, arc_dia_h };

	auto status = gp.Reset();
	qwr::CheckGdi(status, "Reset");

	// top left
	status = gp.AddArc(corner, 180, 90);
	qwr::CheckGdi(status, "AddArc");

	// top right
	corner.X += (rect.Width - arc_dia_w);
	status = gp.AddArc(corner, 270, 90);
	qwr::CheckGdi(status, "AddArc");

	// bottom right
	corner.Y += (rect.Height - arc_dia_h);
	status = gp.AddArc(corner, 0, 90);
	qwr::CheckGdi(status, "AddArc");

	// bottom left
	corner.X -= (rect.Width - arc_dia_w);
	status = gp.AddArc(corner, 90, 90);
	qwr::CheckGdi(status, "AddArc");

	status = gp.CloseFigure();
	qwr::CheckGdi(status, "CloseFigure");
}

void JsGdiGraphics::ParsePoints(JS::HandleValue jsValue, std::vector<Gdiplus::PointF>& gdiPoints)
{
	bool isX = true;
	float x = 0.0;
	auto pointParser = [&gdiPoints, &isX, &x](float coordinate) {
		if (isX)
		{
			x = coordinate;
		}
		else
		{
			gdiPoints.emplace_back(Gdiplus::PointF{ x, coordinate });
		}

		isX = !isX;
	};

	gdiPoints.clear();
	convert::to_native::ProcessArray<float>(pJsCtx_, jsValue, pointParser);

	QwrException::ExpectTrue(isX, "Points count must be a multiple of two"); ///< Means that we were expecting `y` coordinate
}

} // namespace mozjs
