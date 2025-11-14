#pragma once
#include <resources/resource.h>

namespace smp::ui
{

class CDialogEditorConfig : public CDialogImpl<CDialogEditorConfig>, private IListControlOwnerDataSource
{
public:
	enum
	{
		IDD = IDD_DIALOG_EDITOR_CONFIG
	};

	BEGIN_MSG_MAP(CDialogEditorConfig)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_ID_HANDLER_EX(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCloseCmd)
		COMMAND_HANDLER_EX(IDC_BUTTON_RESET, BN_CLICKED, OnButtonReset)
		COMMAND_HANDLER_EX(IDC_BUTTON_EXPORT, BN_CLICKED, OnButtonExportBnClicked)
		COMMAND_HANDLER_EX(IDC_BUTTON_IMPORT, BN_CLICKED, OnButtonImportBnClicked)
	END_MSG_MAP()

	CDialogEditorConfig();

private:
	BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	void OnButtonReset(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	void OnButtonExportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	void OnButtonImportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl);

private:
	bool listIsColumnEditable(ctx_t, size_t column) final;
	size_t listGetItemCount(ctx_t) final;
	pfc::string8 listGetSubItemText(ctx_t, size_t row, size_t column) final;
	void listSetEditField(ctx_t, size_t row, size_t column, const char* value) final;
	void listSubItemClicked(ctx_t, size_t row, size_t column) final;

	CListControlOwnerData m_list;
};

} // namespace smp::ui
