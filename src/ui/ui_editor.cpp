#include <stdafx.h>

#include "ui_editor.h"

#include <panel/js_panel_window.h>
#include <ui/ui_editor_config.h>

#include <2K3/FileDialog.hpp>
#include <2K3/TextFile.hpp>

namespace
{

WINDOWPLACEMENT g_WindowPlacement{};

} // namespace

namespace smp::ui
{

CEditor::CEditor(const std::string& caption, std::string& text, SaveCallback callback)
	: callback_(callback)
	, text_(text)
	, caption_(caption)
{
}

LRESULT CEditor::OnInitDialog(HWND, LPARAM)
{
	menu = GetMenu();
	assert(menu.m_hMenu);

	DlgResize_Init();

	// Apply window placement
	if (!g_WindowPlacement.length)
	{
		WINDOWPLACEMENT tmpPlacement{};
		tmpPlacement.length = sizeof(WINDOWPLACEMENT);

		if (GetWindowPlacement(&tmpPlacement))
		{
			g_WindowPlacement = tmpPlacement;
		}
	}
	else
	{
		SetWindowPlacement(&g_WindowPlacement);
	}

	// Edit Control
	sciEditor_.SubclassWindow(GetDlgItem(IDC_EDIT));
	ReloadProperties();
	sciEditor_.SetContent(text_.c_str(), true);
	sciEditor_.SetSavePoint();

	UpdateUiElements();

	return TRUE; // set focus to default control
}

LRESULT CEditor::OnCloseCmd(WORD, WORD wID, HWND)
{
	// Window position
	{
		WINDOWPLACEMENT tmpPlacement{};
		tmpPlacement.length = sizeof(WINDOWPLACEMENT);
		if (GetWindowPlacement(&tmpPlacement))
		{
			g_WindowPlacement = tmpPlacement;
		}
	}

	switch (wID)
	{
	case IDOK:
	{
		Apply();
		EndDialog(IDOK);
		break;
	}
	case IDAPPLY:
	{
		Apply();
		break;
	}
	case IDCANCEL:
	case ID_APP_EXIT:
	{
		if (sciEditor_.GetModify())
		{
			const int ret = popup_message_v3::get()->messageBox(
				*this,
				"Do you want to apply your changes?",
				caption_.c_str(),
				MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNOCANCEL
			);

			switch (ret)
			{
			case IDYES:
				Apply();
				EndDialog(IDOK);
				break;
			case IDCANCEL:
				return 0;
			default:
				break;
			}
		}

		EndDialog(wID);
		break;
	}
	default:
	{
		assert(0);
	}
	}

	return 0;
}

LRESULT CEditor::OnNotify(int, LPNMHDR pnmh)
{
	// SCNotification* notification = reinterpret_cast<SCNotification*>(pnmh);

	switch (pnmh->code)
	{
	case SCN_SAVEPOINTLEFT:
	{ // dirty
		isDirty_ = true;
		UpdateUiElements();
		break;
	}
	case SCN_SAVEPOINTREACHED:
	{ // not dirty
		isDirty_ = false;
		UpdateUiElements();
		break;
	}
	}

	SetMsgHandled(FALSE);
	return 0;
}

LRESULT CEditor::OnUwmKeyDown(UINT, WPARAM wParam, LPARAM, BOOL& bHandled)
{
	const auto vk = (uint32_t)wParam;
	bHandled = BOOL(ProcessKey(vk) || sciEditor_.ProcessKey(vk));
	return (bHandled ? 0 : 1);
}

LRESULT CEditor::OnFileSave(WORD, WORD, HWND)
{
	Apply();
	return 0;
}

LRESULT CEditor::OnFileImport(WORD, WORD, HWND)
{
	auto path_func = [this](fb2k::stringRef path)
		{
			const auto native = filesystem::g_get_native_path(path->c_str());
			const auto wpath = qwr::ToWide(native);
			const auto str = TextFile(wpath).read();
			sciEditor_.SetContent(str.c_str());
		};

	FileDialog::open(m_hWnd, "Import file", "JavaScript files|*.js|Text files|*.txt|All files|*.*", path_func);
	return 0;
}

LRESULT CEditor::OnFileExport(WORD, WORD, HWND)
{
	auto path_func = [this](fb2k::stringRef path)
		{
			std::string text;
			text.resize(sciEditor_.GetTextLength() + 1);

			sciEditor_.GetText(text.data(), text.size());
			text.resize(strlen(text.data()));

			const auto native = filesystem::g_get_native_path(path->c_str());
			const auto wpath = qwr::ToWide(native);
			TextFile(wpath).write(text);
		};

	FileDialog::save(m_hWnd, "Export file", "JavaScript files|*.js|Text files|*.txt|All files|*.*", "js", path_func);
	return 0;
}

LRESULT CEditor::OnOptionProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/)
{
	CDialogEditorConfig config;
	config.DoModal(m_hWnd);

	ReloadProperties();

	return 0;
}

LRESULT CEditor::OnHelp(WORD, WORD, HWND)
{
	ShellExecute(nullptr, L"open", path::JsDocsIndex().c_str(), nullptr, nullptr, SW_SHOW);
	return 0;
}

LRESULT CEditor::OnAbout(WORD, WORD, HWND)
{
	popup_message_v3::get()->messageBox(*this, about_smp().c_str(), "About Spider Monkey Panel", MB_SETFOREGROUND);
	return 0;
}

void CEditor::ReloadProperties()
{
	sciEditor_.ReloadScintillaSettings();
	sciEditor_.SetJScript();
	sciEditor_.ReadAPI();
}

void CEditor::UpdateUiElements()
{
	if (isDirty_)
	{
		CButton(GetDlgItem(IDAPPLY)).EnableWindow(TRUE);
		uSetWindowText(m_hWnd, (caption_ + " *").c_str());
	}
	else
	{
		CButton(GetDlgItem(IDAPPLY)).EnableWindow(FALSE);
		uSetWindowText(m_hWnd, caption_.c_str());
	}
}

bool CEditor::ProcessKey(uint32_t vk)
{
	const int modifiers = (IsKeyPressed(VK_SHIFT) ? SCMOD_SHIFT : 0)
		| (IsKeyPressed(VK_CONTROL) ? SCMOD_CTRL : 0)
		| (IsKeyPressed(VK_MENU) ? SCMOD_ALT : 0);

	// Hotkeys
	if (modifiers == SCMOD_CTRL && vk == 'S')
	{
		Apply();
		return true;
	}

	return false;
}

void CEditor::Apply()
{
	sciEditor_.SetSavePoint();

	std::vector<char> textBuffer(sciEditor_.GetTextLength() + 1);
	sciEditor_.GetText(textBuffer.data(), textBuffer.size());
	text_ = textBuffer.data();

	if (callback_)
	{
		callback_();
	}
}

} // namespace smp::ui
