#pragma once
#include <com_objects/com_tools.h>

_COM_SMARTPTR_TYPEDEF(IDragSourceHelper, IID_IDragSourceHelper);

namespace smp::com
{
	class IDropSourceImpl : public IDropSource
	{
	public:
		/// @throw qwr::QwrException
		IDropSourceImpl(HWND hWnd, IDataObject* pDataObject, size_t itemCount, bool showText, Gdiplus::Bitmap* pUserImage);
		virtual ~IDropSourceImpl();

		// IDropSource
		STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) override;
		STDMETHODIMP GiveFeedback(DWORD dwEffect) override;
		ULONG STDMETHODCALLTYPE AddRef() override;
		ULONG STDMETHODCALLTYPE Release() override;

	private:
		IDragSourceHelperPtr pDragSourceHelper_;
		IDataObject* pDataObject_{};
		SHDRAGIMAGE dragImage_{};
		bool wasShowingLayered_{};
		std::atomic<ULONG> refCount_{};
		DWORD lastEffect_ = DROPEFFECT_NONE;

		COM_QI_SIMPLE(IDropSource)
	};
}
