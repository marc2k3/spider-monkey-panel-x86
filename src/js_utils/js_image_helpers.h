#pragma once

namespace mozjs::image
{
	[[nodiscard]] JSObject* GetImagePromise(JSContext* cx, HWND hWnd, const std::wstring& imagePath);
}
