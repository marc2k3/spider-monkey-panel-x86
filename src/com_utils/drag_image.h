#pragma once

// Copied from https://github.com/reupen/ui_helpers
// Copyright (c) Reupen Shah 2003-2017
// All rights reserved.
// See THIRD_PARTY_NOTICES.md for full license text.

namespace uih
{
	void draw_drag_image_background(HWND wnd, HTHEME theme, HDC dc, COLORREF selectionBackgroundColour, const RECT& rc);
	void draw_drag_image_label(HWND wnd, HTHEME theme, HDC dc, const RECT& rc, COLORREF selectionTextColour, std::string_view text);
	void draw_drag_image_icon(HDC dc, const RECT& rc, HICON icon);
	bool create_drag_image(HWND wnd, HTHEME theme, COLORREF selectionBackgroundColour,
							COLORREF selectionTextColour, HICON icon, const LPLOGFONT font, std::string_view text,
							Gdiplus::Bitmap* pCustomImage, LPSHDRAGIMAGE lpsdi);
}
