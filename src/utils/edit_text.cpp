#include <stdafx.h>
#include "edit_text.h"

#include <2K3/FileHelper.hpp>
#include <2K3/TextFile.hpp>
#include <panel/modal_blocking_scope.h>
#include <ui/ui_edit_in_progress.h>
#include <ui/ui_editor.h>

namespace
{
	using namespace smp;
	namespace fs = std::filesystem;

	fs::path GetFixedEditorPath()
	{
		const std::string tmp = fb2k::configStore::get()->getConfigString("smp.editor.path")->c_str();
		const auto editorPath = qwr::ToWide(tmp);

		if (FileHelper(editorPath).is_file())
			return editorPath;

		return {};
	}

	void NotifyParentPanel(HWND hParent)
	{
		SendMessageW(hParent, static_cast<INT>(InternalSyncMessage::ui_script_editor_saved), 0, 0);
	}

	void EditTextFileInternal(HWND hParent, const fs::path& file, bool isPanelScript)
	{
		auto text = TextFile(file.native()).read();
		auto scope = modal::ConditionalModalScope(hParent, isPanelScript);
		auto dlg = smp::ui::CEditor(file.filename().u8string(), text, [&]
			{
				TextFile(file.native()).write(text);

				if (isPanelScript)
				{
					NotifyParentPanel(hParent);
				}
			});

		dlg.DoModal(hParent);
	}

	bool EditTextFileExternal(HWND hParent, const fs::path& file, const fs::path& pathToEditor, bool isModal, bool isPanelScript)
	{
		if (isModal)
		{
			modal::ConditionalModalScope scope(hParent, isPanelScript);
			auto dlg = ui::CEditInProgress(pathToEditor, file);
			return dlg.DoModal(hParent) == IDOK;
		}

		const auto qPath = L"\"" + file.wstring() + L"\"";

		const auto hInstance = ShellExecuteW(
			nullptr,
			L"open",
			pathToEditor.c_str(),
			qPath.c_str(),
			nullptr,
			SW_SHOW
		);

		if ((int)hInstance < 32)
		{
			qwr::error::CheckWin32((int)hInstance, "ShellExecute");
		}

		return true;
	}

	void EditTextInternal(HWND hParent, std::string& text, bool isPanelScript)
	{
		modal::ConditionalModalScope scope(hParent, isPanelScript);

		auto dlg = smp::ui::CEditor("Temporary file", text, [&]
			{
				if (isPanelScript)
					NotifyParentPanel(hParent);
			});

		dlg.DoModal(hParent);
	}

	void EditTextExternal(HWND hParent, std::string& text, const fs::path& pathToEditor, bool isPanelScript)
	{
		// keep .tmp for the uniqueness
		const auto fsTmpFilePath = []
			{
				std::wstring tmpFilePath;
				tmpFilePath.resize(MAX_PATH - 14); // max allowed size of path in GetTempFileName

				DWORD dwRet = GetTempPathW(tmpFilePath.size(), tmpFilePath.data());
				qwr::error::CheckWinApi(dwRet && dwRet <= tmpFilePath.size(), "GetTempPath");

				std::wstring filename;
				filename.resize(MAX_PATH);

				const auto uRet = GetTempFileNameW(
					tmpFilePath.c_str(),
					L"smp",
					0,
					filename.data()
				);

				qwr::error::CheckWinApi(uRet, "GetTempFileName");
				filename.resize(wcslen(filename.c_str()));
				return fs::path(tmpFilePath) / filename;
			}();

		// use .tmp.js for proper file association
		const auto fsJsTmpFilePath = fs::path(fsTmpFilePath).concat(L".js");

		TextFile(fsJsTmpFilePath.native()).write(text);

		if (EditTextFileExternal(hParent, fsJsTmpFilePath, pathToEditor, true, isPanelScript))
		{
			text = TextFile(fsJsTmpFilePath.native()).read();
		}

		std::error_code ec;
		fs::remove(fsTmpFilePath, ec);
		fs::remove(fsJsTmpFilePath, ec);
	}
}

namespace smp
{
	void EditTextFile(HWND hParent, const fs::path& file, bool isPanelScript, bool isModal)
	{
		const auto editorPath = GetFixedEditorPath();

		if (editorPath.empty())
		{
			EditTextFileInternal(hParent, file, isPanelScript);
		}
		else
		{
			EditTextFileExternal(hParent, file, editorPath, isModal, isPanelScript);

			if (isPanelScript)
			{
				NotifyParentPanel(hParent);
			}
		}
	}

	void EditText(HWND hParent, std::string& text, bool isPanelScript)
	{
		const auto editorPath = GetFixedEditorPath();

		if (editorPath.empty())
		{
			EditTextInternal(hParent, text, isPanelScript);
		}
		else
		{
			EditTextExternal(hParent, text, editorPath, isPanelScript);

			if (isPanelScript)
			{
				NotifyParentPanel(hParent);
			}
		}
	}
}
