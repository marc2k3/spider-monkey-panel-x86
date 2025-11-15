#include <stdafx.h>
#include "ui_input_box.h"

#include <utils/gdi_helpers.h>

namespace smp::ui
{
	CInputBox::CInputBox(std::string_view prompt, std::string_view caption, std::string_view value)
		: prompt_(prompt)
		, caption_(caption)
		, value_(value) {
	}

	LRESULT CInputBox::OnInitDialog(HWND /*hwndFocus*/, LPARAM /*lParam*/)
	{
		DlgResize_Init(false);

		AdjustPromptControlToFit();

		// Disable manual resizing
		SetWindowLongW(GWL_STYLE, GetWindowLong(GWL_STYLE) & ~WS_THICKFRAME);

		uSetWindowText(m_hWnd, caption_.c_str());
		uSendDlgItemMessageText(m_hWnd, IDC_INPUT_PROMPT, WM_SETTEXT, 0, prompt_.c_str());
		uSendDlgItemMessageText(m_hWnd, IDC_INPUT_VALUE, WM_SETTEXT, 0, value_.c_str());

		// Select all
		SendDlgItemMessageW(IDC_INPUT_VALUE, EM_SETSEL, 0, -1);
		::SetFocus(GetDlgItem(IDC_INPUT_VALUE));
		CenterWindow();

		hooks_.AddDialogWithControls(m_hWnd);
		return FALSE;
	}

	LRESULT CInputBox::OnCloseCmd(UINT /*codeNotify*/, int id, HWND /*hwndCtl*/)
	{
		if (id == IDOK)
		{
			value_ = pfc::getWindowText(GetDlgItem(IDC_INPUT_VALUE)).get_ptr();
		}

		EndDialog(id);
		return 0;
	}

	std::string CInputBox::GetValue()
	{
		return value_;
	}

	void CInputBox::AdjustPromptControlToFit()
	{
		CStatic promptCtrl(GetDlgItem(IDC_INPUT_PROMPT));
		RECT promptRc{};
		promptCtrl.GetClientRect(&promptRc);

		RECT newPromptRc = promptRc;
		CPaintDC dc(m_hWnd);
		HDC hDc = dc.m_hDC;

		auto _ = wil::SelectObject(hDc, promptCtrl.GetFont());
		const auto promptW = qwr::ToWide(prompt_);
		DrawTextW(hDc, const_cast<wchar_t*>(promptW.c_str()), -1, &newPromptRc, DT_CALCRECT | DT_NOPREFIX | DT_WORDBREAK);

		if (newPromptRc.bottom > promptRc.bottom || newPromptRc.right > promptRc.right)
		{
			WINDOWPLACEMENT placement{};
			placement.length = sizeof(WINDOWPLACEMENT);

			if (GetWindowPlacement(&placement))
			{
				if (newPromptRc.bottom > promptRc.bottom)
				{
					placement.rcNormalPosition.bottom += newPromptRc.bottom - promptRc.bottom;
				}

				if (newPromptRc.right > promptRc.right)
				{
					placement.rcNormalPosition.right += newPromptRc.right - promptRc.right;
				}

				SetWindowPlacement(&placement);
			}
		}
	}
}
