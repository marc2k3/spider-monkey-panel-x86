#pragma once

namespace smp::utils
{
	struct WrappedTextLine
	{
		std::wstring text;
		size_t width;
	};

	[[nodiscard]] size_t GetTextHeight(HDC hdc, std::wstring_view text);
	[[nodiscard]] size_t GetTextWidth(HDC hdc, std::wstring_view text, bool accurate = false);
	[[nodiscard]] std::vector<WrappedTextLine> WrapText(HDC hdc, const std::wstring& text, size_t maxWidth);
}
