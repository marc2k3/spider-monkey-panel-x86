#pragma once

#define DDWM_SETCURSOR    (WM_USER + 2)
#define DDWM_UPDATEWINDOW (WM_USER + 3)

namespace smp::com::drag
{
	HRESULT GetIsShowingLayered(IDataObject* pDataObj, BOOL& p_out);
	HRESULT SetDefaultImage(IDataObject* pdtobj);
	HRESULT SetDropText(IDataObject* pdtobj, DROPIMAGETYPE dit, const wchar_t* msg, const wchar_t* insert);
	HRESULT GetDragWindow(IDataObject* pDataObj, HWND& p_wnd);
	bool RenderDragImage(HWND hWnd, size_t itemCount, bool showText, Gdiplus::Bitmap* pCustomImage, SHDRAGIMAGE& dragImage);
}
