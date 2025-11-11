#include <stdafx.h>
#include "drag_utils.h"

#include <com_utils/drag_image.h>
#include <utils/gdi_helpers.h>

namespace
{
	template <typename T>
	HRESULT GetDataObjectDataSimple(IDataObject* pDataObj, CLIPFORMAT cf, T& p_out)
	{
		FORMATETC fmte = { cf, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM stgm{};

		RETURN_IF_FAILED(pDataObj->GetData(&fmte, &stgm));

		if (void* pData = GlobalLock(stgm.hGlobal); pData)
		{
			p_out = *static_cast<T*>(pData);
			GlobalUnlock(pData);
		}

		ReleaseStgMedium(&stgm);
		return S_OK;
	}

	HRESULT SetDataBlob(IDataObject* pdtobj, CLIPFORMAT cf, const void* pvBlob, UINT cbBlob)
	{
		auto pv = GlobalAlloc(GPTR, cbBlob);

		if (!pv)
		{
			return E_OUTOFMEMORY;
		}

		CopyMemory(pv, pvBlob, cbBlob);

		FORMATETC fmte = { cf, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM stgm{};
		stgm.tymed = TYMED_HGLOBAL;
		stgm.hGlobal = pv;

		const auto hr = pdtobj->SetData(&fmte, &stgm, TRUE);

		if FAILED(hr)
		{
			GlobalFree(pv);
		}

		return hr;
	}

	std::string FormatDragText(t_size selectionCount)
	{
		return fmt::format("{} {}", selectionCount, (selectionCount > 1 ? "tracks" : "track"));
	}
}

namespace smp::com::drag
{
	HRESULT SetDefaultImage(IDataObject* pdtobj)
	{
		static const auto cfRet = static_cast<CLIPFORMAT>(RegisterClipboardFormatW(L"UsingDefaultDragImage"));
		const BOOL blobValue = TRUE;
		return SetDataBlob(pdtobj, cfRet, &blobValue, sizeof(blobValue));
	}

	HRESULT SetDropText(IDataObject* pdtobj, DROPIMAGETYPE dit, const wchar_t* msg, const wchar_t* insert)
	{
		static const auto cfRet = static_cast<CLIPFORMAT>(RegisterClipboardFormatW(CFSTR_DROPDESCRIPTION));

		DROPDESCRIPTION dd_prev{};

		const auto hr = GetDataObjectDataSimple(pdtobj, cfRet, dd_prev);

		// Only set the drop description if it has actually changed (otherwise things get a bit crazy near the edge of
		// the screen).
		if (FAILED(hr) || dd_prev.type != dit || wcscmp(dd_prev.szInsert, insert) || wcscmp(dd_prev.szMessage, msg))
		{
			DROPDESCRIPTION dd{};
			dd.type = dit;
			wcscpy_s(dd.szMessage, msg);
			wcscpy_s(dd.szInsert, insert);
			return SetDataBlob(pdtobj, cfRet, &dd, sizeof(dd));
		}

		return S_OK;
	}

	HRESULT GetDragWindow(IDataObject* pDataObj, HWND& p_wnd)
	{
		static const auto cfRet = static_cast<CLIPFORMAT>(RegisterClipboardFormatW(L"DragWindow"));
		DWORD dw;
		RETURN_IF_FAILED(GetDataObjectDataSimple(pDataObj, cfRet, dw));

		p_wnd = static_cast<HWND>(ULongToHandle(dw));
		return S_OK;
	}

	HRESULT GetIsShowingLayered(IDataObject* pDataObj, BOOL& p_out)
	{
		static const auto cfRet = static_cast<CLIPFORMAT>(RegisterClipboardFormatW(L"IsShowingLayered"));
		return GetDataObjectDataSimple(pDataObj, cfRet, p_out);
	}

	bool RenderDragImage(HWND hWnd, size_t itemCount, bool showText, Gdiplus::Bitmap* pCustomImage, SHDRAGIMAGE& dragImage)
	{
		wil::unique_htheme dd_theme; 
		
		if (IsThemeActive() && IsAppThemed())
		{
			dd_theme.reset(OpenThemeData(hWnd, VSCLASS_DRAGDROP));
		}

		LOGFONT lf;
		memset(&lf, 0, sizeof(LOGFONT));
		SystemParametersInfoW(SPI_GETICONTITLELOGFONT, 0, &lf, 0);

		return uih::create_drag_image(
			hWnd,
			dd_theme.get(),
			GetSysColor(COLOR_HIGHLIGHT),
			GetSysColor(COLOR_HIGHLIGHTTEXT),
			nullptr,
			&lf,
			showText ? FormatDragText(itemCount) : "",
			pCustomImage,
			&dragImage
		);
	}
}
