#include <stdafx.h>
#include "drag_image.h"
#include <utils/image_helpers.h>

namespace
{
	/// Restore some windowsx.h macros
	#ifndef SelectFont
	#define SelectFont(hdc, hfont) ((HFONT)SelectObject((hdc), (HGDIOBJ)(HFONT)(hfont)))
	#endif
	#ifndef SelectBitmap
	#define SelectBitmap(hdc, hbm) ((HBITMAP)SelectObject((hdc), (HGDIOBJ)(HBITMAP)(hbm)))
	#endif

	// Ripped from win32_helpers.cpp

	SIZE get_system_dpi()
	{
		HDC dc = GetDC(nullptr);
		SIZE ret = { GetDeviceCaps(dc, LOGPIXELSX), GetDeviceCaps(dc, LOGPIXELSY) };
		ReleaseDC(nullptr, dc);
		return ret;
	}

	SIZE get_system_dpi_cached()
	{
		static const SIZE size = get_system_dpi();
		return size;
	}

	bool UsesTheming(HTHEME theme, int partId, int stateId)
	{
		return theme && IsThemePartDefined(theme, partId, stateId);
	}
}

namespace uih
{
	// Only used in non-themed mode – if theming is active, the shell draws the background for us
	void draw_drag_image_background(HWND wnd, HTHEME theme, HDC dc, COLORREF selectionBackgroundColour, const RECT& rc)
	{
		constexpr int themeState = 0;

		if (UsesTheming(theme, DD_IMAGEBG, themeState))
		{
			if (IsThemeBackgroundPartiallyTransparent(theme, DD_IMAGEBG, themeState))
			{
				DrawThemeParentBackground(wnd, dc, &rc);
			}
			DrawThemeBackground(theme, dc, DD_IMAGEBG, themeState, &rc, nullptr);
		}
		else
		{
			auto brush = wil::unique_hbrush(CreateSolidBrush(selectionBackgroundColour));
			FillRect(dc, &rc, brush.get());
		}
	}

	void draw_drag_image_label(HWND wnd, HTHEME theme, HDC dc, const RECT& rc, COLORREF selectionTextColour, std::string_view text)
	{
		constexpr int theme_state = 0;
		const bool useTheming = UsesTheming(theme, DD_TEXTBG, theme_state);
		const auto wtext = qwr::ToWide(text);

		DWORD text_flags = DT_CENTER | DT_WORDBREAK;
		RECT rc_text{};

		if (useTheming)
		{
			GetThemeTextExtent(theme, dc, DD_TEXTBG, theme_state, wtext.c_str(), wtext.length(), text_flags, &rc, &rc_text);
		}
		else
		{
			rc_text = rc;
			DrawTextW(dc, wtext.c_str(), wtext.length(), &rc_text, text_flags | DT_CALCRECT);
		}

		auto x_offset = (wil::rect_width(rc) - wil::rect_width(rc_text)) / 2;
		auto y_offset = (wil::rect_height(rc) - wil::rect_height(rc_text)) / 2;
		rc_text.left += x_offset;
		rc_text.right += x_offset;
		rc_text.top += y_offset;
		rc_text.bottom += y_offset;

		if (useTheming)
		{
			MARGINS margins{};
			GetThemeMargins(theme, dc, DD_TEXTBG, theme_state, TMT_CONTENTMARGINS, &rc_text, &margins);

			RECT background_rect = rc_text;
			background_rect.left -= margins.cxLeftWidth;
			background_rect.top -= margins.cyTopHeight;
			background_rect.bottom += margins.cyBottomHeight;
			background_rect.right += margins.cxRightWidth;

			if (IsThemeBackgroundPartiallyTransparent(theme, DD_TEXTBG, 0))
			{
				DrawThemeParentBackground(wnd, dc, &background_rect);
			}
			DrawThemeBackground(theme, dc, DD_TEXTBG, theme_state, &background_rect, nullptr);
			DrawThemeText(theme, dc, DD_TEXTBG, theme_state, wtext.c_str(), wtext.length(), text_flags, 0, &rc_text);
		}
		else
		{
			auto previousColour = GetTextColor(dc);
			auto previousBackgroundMode = GetBkMode(dc);
			SetTextColor(dc, selectionTextColour);
			SetBkMode(dc, TRANSPARENT);
			DrawTextW(dc, wtext.c_str(), wtext.length(), &rc_text, text_flags);
			SetTextColor(dc, previousColour);
			SetBkMode(dc, previousBackgroundMode);
		}
	}

	bool draw_drag_custom_image(HDC dc, const RECT& rc, Gdiplus::Bitmap& customImage)
	{
		const int imgWidth = static_cast<int>(customImage.GetWidth());
		const int imgHeight = static_cast<int>(customImage.GetHeight());

		const auto [newWidth, newHeight] = [imgWidth, imgHeight, &rc]
			{
				return smp::image::GetResizedImageSize(std::make_tuple(imgWidth, imgHeight), std::make_tuple(rc.right, rc.bottom));
			}();

		auto gdiGraphics = Gdiplus::Graphics(dc);

		const auto status = gdiGraphics.DrawImage(
			&customImage,
			Gdiplus::Rect{
				lround(static_cast<float>(rc.right - newWidth) / 2),
				lround(static_cast<float>(rc.bottom - newHeight) / 2),
				static_cast<int>(newWidth),
				static_cast<int>(newHeight)
			},
			0,
			0,
			imgWidth,
			imgHeight,
			Gdiplus::UnitPixel
		);

		return (Gdiplus::Ok == status);
	}

	void draw_drag_image_icon(HDC dc, const RECT& rc, HICON icon)
	{
		// We may want to use better scaling.
		DrawIconEx(dc, 0, 0, icon, wil::rect_width(rc), wil::rect_height(rc), 0, nullptr, DI_NORMAL);
	}

	std::tuple<SIZE, POINT> GetDragImageContentSizeAndOffset(HDC dc, HTHEME theme)
	{
		constexpr int themeState = 0;

		auto sz = get_system_dpi_cached();
		POINT offset{};

		if (!UsesTheming(theme, DD_IMAGEBG, themeState))
		{
			return { sz, offset };
		}

		if FAILED(GetThemePartSize(theme, dc, DD_IMAGEBG, themeState, nullptr, TS_DRAW, &sz))
		{
			return { sz, offset };
		}

		MARGINS margins{};

		if SUCCEEDED(GetThemeMargins(theme, dc, DD_IMAGEBG, themeState, TMT_CONTENTMARGINS, nullptr, &margins))
		{
			sz.cx -= margins.cxLeftWidth + margins.cxRightWidth;
			sz.cy -= margins.cyBottomHeight + margins.cyTopHeight;
		}

		return { sz, offset };
	}

	bool create_drag_image(HWND wnd, HTHEME theme, COLORREF selectionBackgroundColour,
							COLORREF selectionTextColour, HICON icon, const LPLOGFONT font, std::string_view text,
							Gdiplus::Bitmap* pCustomImage, LPSHDRAGIMAGE lpsdi)
	{
		HDC dc = GetDC(wnd);
		HDC dc_mem = CreateCompatibleDC(dc);

		auto [size, offset] = GetDragImageContentSizeAndOffset(dc, theme);
		const RECT rc{ 0, 0, size.cx, size.cy };

		HBITMAP bm_mem = CreateCompatibleBitmap(dc, size.cx, size.cy);
		HBITMAP bm_old = SelectBitmap(dc_mem, bm_mem);

		LOGFONT lf = *font;
		lf.lfWeight = FW_BOLD;
		// lf.lfQuality = NONANTIALIASED_QUALITY;

		HFONT fnt = CreateFontIndirectW(&lf);
		HFONT font_old = SelectFont(dc_mem, fnt);

		if (pCustomImage)
		{
			if (!draw_drag_custom_image(dc_mem, rc, *pCustomImage))
			{
				return false;
			}
		}
		else
		{
			if (!theme)
			{
				draw_drag_image_background(wnd, theme, dc_mem, selectionBackgroundColour, rc);
			}
		}

		if (icon)
		{
			draw_drag_image_icon(dc_mem, rc, icon);
		}

		if (text.length())
		{
			draw_drag_image_label(wnd, theme, dc_mem, rc, selectionTextColour, text);
		}

		SelectFont(dc_mem, font_old);
		DeleteFont(fnt);

		SelectObject(dc_mem, bm_old);
		DeleteDC(dc_mem);
		ReleaseDC(wnd, dc);

		lpsdi->sizeDragImage.cx = size.cx;
		lpsdi->sizeDragImage.cy = size.cy;
		lpsdi->ptOffset.x = size.cx / 2;
		lpsdi->ptOffset.y = (size.cy - offset.y) - (size.cy - offset.y) / 10;
		lpsdi->hbmpDragImage = bm_mem;
		lpsdi->crColorKey = CLR_NONE;

		return true;
	}
}
