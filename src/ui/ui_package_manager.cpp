#include <stdafx.h>

#include "ui_package_manager.h"

#include <config/package_utils.h>
#include <ui/ui_input_box.h>
#include <utils/zip_utils.h>

#include <2K3/FileDialog.hpp>
#include <qwr/error_popup.h>
#include <qwr/winapi_error_helpers.h>

namespace fs = std::filesystem;

namespace smp::ui
{

CDialogPackageManager::CDialogPackageManager(const std::string& currentPackageId)
	: focusedPackageId_(currentPackageId)
	, ddx_({ qwr::ui::CreateUiDdx<qwr::ui::UiDdx_ListBox>(focusedPackageIdx_, IDC_LIST_PACKAGES) })
{
}

std::optional<config::ParsedPanelSettings>
CDialogPackageManager::GetPackage() const
{
	if (focusedPackageIdx_ < 0)
	{
		return std::nullopt;
	}
	else
	{
		return packages_[focusedPackageIdx_].parsedSettings;
	}
}

LRESULT CDialogPackageManager::OnInitDialog(HWND, LPARAM)
{
	for (auto& ddx: ddx_)
	{
		ddx->SetHwnd(m_hWnd);
	}

	packagesListBox_ = GetDlgItem(IDC_LIST_PACKAGES);
	pPackagesListBoxDrop_.Attach(new com::ComPtrImpl<com::FileDropTarget>(packagesListBox_, *this));

	try
	{
		HRESULT hr = pPackagesListBoxDrop_->RegisterDragDrop();
		qwr::error::CheckHR(hr, "RegisterDragDrop");
	}
	catch (qwr::QwrException& e)
	{
		qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, e.what());
	}

	// TODO: add context menu
	packageInfoEdit_ = GetDlgItem(IDC_RICHEDIT_PACKAGE_INFO);
	packageInfoEdit_.SetWindowLong(GWL_EXSTYLE, packageInfoEdit_.GetWindowLong(GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
	packageInfoEdit_.SetEditStyle(SES_HYPERLINKTOOLTIPS | SES_NOFOCUSLINKNOTIFY, SES_HYPERLINKTOOLTIPS | SES_NOFOCUSLINKNOTIFY);
	packageInfoEdit_.SetAutoURLDetect();
	packageInfoEdit_.SetEventMask(packageInfoEdit_.GetEventMask() | ENM_LINK);

	SetWindowText(L"Script package manager");

	CenterWindow();
	::SetFocus(packagesListBox_);

	LoadPackages();
	UpdateListBoxFromData();
	DoFullDdxToUi();

	return FALSE;
}

void CDialogPackageManager::OnDestroy()
{
	assert(pPackagesListBoxDrop_);
	pPackagesListBoxDrop_->RevokeDragDrop();
	pPackagesListBoxDrop_.Release();
}

void CDialogPackageManager::OnDdxUiChange(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
	auto it = ranges::find_if(ddx_, [nID](auto& ddx) {
		return ddx->IsMatchingId(nID);
	});

	if (ddx_.end() != it)
	{
		(*it)->ReadFromUi();
	}

	if (nID == IDC_LIST_PACKAGES)
	{
		UpdatedUiPackageInfo();
		UpdateUiButtons();
	}
}

void CDialogPackageManager::OnNewPackage(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	std::string curName;
	while (true)
	{
		CInputBox dlg("Enter new package name", "Creating new package", curName.c_str());
		if (dlg.DoModal(m_hWnd) != IDOK)
		{
			return;
		}

		curName = dlg.GetValue();
		if (curName.empty())
		{
			popup_message_v3::get()->messageBox(
				*this,
				"Can't create package with empty name",
				"Creating new package",
				MB_OK | MB_ICONWARNING);
			continue;
		}

		break;
	};

	try
	{
		const auto newSettings = config::GetNewPackageSettings(curName);
		config::MaybeSavePackageData(newSettings);

		packages_.emplace_back(GeneratePackageData(newSettings));
		focusedPackageId_ = *newSettings.packageId;

		UpdateListBoxFromData();
		DoFullDdxToUi();
	}
	catch (const qwr::QwrException& e)
	{
		qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, e.what());
	}
}

void CDialogPackageManager::OnDeletePackage(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	if (packages_.empty())
	{
		return;
	}

	assert(focusedPackageIdx_ >= 0 && static_cast<size_t>(focusedPackageIdx_) < packages_.size());

	const int iRet = popup_message_v3::get()->messageBox(
		*this, "Are you sure you want to delete the package?", "Deleting package", MB_YESNO);
	if (iRet != IDYES)
	{
		return;
	}

	try
	{
		const auto packageId = packages_[focusedPackageIdx_].id;
		const auto packagePathOpt = config::FindPackage(packages_[focusedPackageIdx_].id);
		if (packagePathOpt)
		{
			if (config::IsPackageInUse(packageId))
			{
				config::MarkPackageAsToBeRemoved(packageId);

				auto it = ranges::find_if(packages_, [&](const auto& elem)
					{
						return (packageId == elem.id);
					});

				if (it != packages_.cend())
				{
					focusedPackageId_ = packageId;
					it->status = config::PackageDelayStatus::ToBeRemoved;
				}

				if (ConfirmRebootOnPackageInUse())
				{
					standard_commands::main_restart();
				}

				UpdateListBoxFromData();
				DoFullDdxToUi();
				return;
			}

			fs::remove_all(*packagePathOpt);
		}
	}
	catch (const fs::filesystem_error& e)
	{
		qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, qwr::ToU8_FromAcpToWide(e.what()));
	}
	catch (const qwr::QwrException& e)
	{
		qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, e.what());
	}

	packages_.erase(packages_.cbegin() + focusedPackageIdx_);
	focusedPackageIdx_ = (packages_.empty() ? -1 : std::max(0, focusedPackageIdx_ - 1));
	focusedPackageId_ = (focusedPackageIdx_ == -1 ? std::string{} : packages_[focusedPackageIdx_].id);

	UpdateListBoxFromData();
	DoFullDdxToUi();
}

void CDialogPackageManager::OnImportPackage(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	auto path_func = [this](fb2k::stringRef path)
		{
			const auto native = filesystem::g_get_native_path(path->c_str());
			const auto wpath = qwr::ToWide(native);
			const auto isRestartNeeded = ImportPackage(wpath);

			if (isRestartNeeded && ConfirmRebootOnPackageInUse())
			{
				standard_commands::main_restart();
			}
		};

	FileDialog::open(m_hWnd, "Import package", "Zip archives|*.zip", path_func);
}

void CDialogPackageManager::OnExportPackage(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	assert(focusedPackageIdx_ >= 0 && static_cast<size_t>(focusedPackageIdx_) < packages_.size());
	assert(packages_[focusedPackageIdx_].parsedSettings);

	auto path_func = [this](fb2k::stringRef path)
		{
			const auto native = filesystem::g_get_native_path(path->c_str());
			const auto wpath = qwr::ToWide(native);
			const auto& currentPackageData = packages_[focusedPackageIdx_];

			try
			{
				auto zp = ZipPacker(wpath);
				zp.AddFolder(config::GetPackagePath(*currentPackageData.parsedSettings));
				zp.Finish();
			}
			catch (const fs::filesystem_error& e)
			{
				qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, qwr::ToU8_FromAcpToWide(e.what()));
			}
			catch (const qwr::QwrException& e)
			{
				qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, e.what());
			}
		};

	FileDialog::save(m_hWnd, "Save package as", "Zip archives|*.zip", "zip", path_func);
}

void CDialogPackageManager::OnOpenFolder(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	assert(focusedPackageIdx_ >= 0 && static_cast<size_t>(focusedPackageIdx_) < packages_.size());
	assert(packages_[focusedPackageIdx_].parsedSettings);

	try
	{
		const auto hInstance = ShellExecute(
			nullptr,
			L"explore",
			config::GetPackagePath(*packages_[focusedPackageIdx_].parsedSettings).c_str(),
			nullptr,
			nullptr,
			SW_SHOWNORMAL
		);

		if ((int)hInstance < 32)
		{ // As per WinAPI
			qwr::error::CheckWin32((int)hInstance, "ShellExecute");
		}
	}
	catch (const fs::filesystem_error& e)
	{
		qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, qwr::ToU8_FromAcpToWide(e.what()));
	}
	catch (const qwr::QwrException& e)
	{
		qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, e.what());
	}
}

LRESULT CDialogPackageManager::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/)
{
	if (wID != IDOK)
	{
		focusedPackageIdx_ = -1;
	}
	EndDialog(wID);
	return 0;
}

LRESULT CDialogPackageManager::OnRichEditLinkClick(LPNMHDR pnmh)
{
	const auto* pEl = reinterpret_cast<ENLINK*>(pnmh);
	if (pEl->msg == WM_LBUTTONUP)
	{
		std::wstring url;
		url.resize(pEl->chrg.cpMax - pEl->chrg.cpMin);
		packageInfoEdit_.GetTextRange(pEl->chrg.cpMin, pEl->chrg.cpMax, url.data());

		ShellExecute(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	}

	return 0;
}

LRESULT CDialogPackageManager::OnDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam)
{
	bool isRestartNeeded = false;
	const auto result = pPackagesListBoxDrop_->ProcessMessage(
		packagesListBox_,
		wParam,
		lParam,
		[&](const auto& path) { isRestartNeeded |= ImportPackage(path); });
	if (isRestartNeeded && ConfirmRebootOnPackageInUse())
	{
		standard_commands::main_restart();
	}

	return result;
}

void CDialogPackageManager::DoFullDdxToUi()
{
	if (!this->m_hWnd)
	{
		return;
	}

	for (auto& ddx: ddx_)
	{
		ddx->WriteToUi();
	}

	UpdatedUiPackageInfo();
	UpdateUiButtons();
}

void CDialogPackageManager::UpdateUiButtons()
{
	if (focusedPackageIdx_ < 0 || static_cast<size_t>(focusedPackageIdx_) >= packages_.size())
	{
		CButton{ GetDlgItem(IDOK) }.EnableWindow(false);
		CButton{ GetDlgItem(IDC_BUTTON_DELETE_PACKAGE) }.EnableWindow(false);
		CButton{ GetDlgItem(IDC_BUTTON_EXPORT_PACKAGE) }.EnableWindow(false);
		CButton{ GetDlgItem(IDC_BUTTON_OPEN_FOLDER) }.EnableWindow(false);
		return;
	}

	const auto& currentPackageData = packages_[focusedPackageIdx_];
	const auto isSample = (currentPackageData.parsedSettings && currentPackageData.parsedSettings->isSample);

	CButton{ GetDlgItem(IDOK) }.EnableWindow(!!currentPackageData.parsedSettings);
	CButton{ GetDlgItem(IDC_BUTTON_DELETE_PACKAGE) }.EnableWindow(currentPackageData.status != config::PackageDelayStatus::ToBeRemoved && !isSample);
	CButton{ GetDlgItem(IDC_BUTTON_EXPORT_PACKAGE) }.EnableWindow(!isSample);
	CButton{ GetDlgItem(IDC_BUTTON_OPEN_FOLDER) }.EnableWindow(!!currentPackageData.parsedSettings);
}

void CDialogPackageManager::LoadPackages()
{
	packages_.clear();

	try
	{
		// TODO: consider extracting this code
		std::vector<fs::path> packagesDirs{ path::Packages_Profile(),
											path::Packages_Sample() };
		if (smp::path::Profile() != smp::path::Foobar2000())
		{ // these paths might be the same when fb2k is in portable mode
			packagesDirs.emplace_back(path::Packages_Foobar2000());
		}

		Strings packageIds;
		for (const auto& packagesDir: packagesDirs)
		{
			if (!fs::exists(packagesDir))
			{
				continue;
			}

			for (const auto& dirIt: fs::directory_iterator(packagesDir))
			{
				const auto packageJson = dirIt.path() / L"package.json";
				if (!fs::exists(packageJson) || !fs::is_regular_file(packageJson))
				{
					continue;
				}

				packageIds.emplace_back(dirIt.path().filename().u8string());
			}
		}

		std::vector<PackageData> parsedPackages;
		for (const auto& packageId: packageIds)
		{
			try
			{
				const auto packagePathOpt = config::FindPackage(packageId);
				qwr::QwrException::ExpectTrue(packagePathOpt.has_value(), "Could not find package with id: {}", packageId);

				const auto settings = config::GetPackageSettingsFromPath(*packagePathOpt);
				parsedPackages.emplace_back(GeneratePackageData(settings));
			}
			catch (const qwr::QwrException& e)
			{
				PackageData packageData{
					qwr::ToWide(fmt::format("{} (ERROR)", packageId)),
					packageId,
					std::nullopt,
					qwr::ToWide(fmt::format("Package parsing failed:\r\n{}", e.what()))
				};

				parsedPackages.emplace_back(packageData);
			}

			parsedPackages.back().status = config::GetPackageDelayStatus(packageId);
		}

		packages_ = std::move(parsedPackages);
	}
	catch (const fs::filesystem_error& e)
	{
		qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, qwr::ToU8_FromAcpToWide(e.what()));
	}
}

void CDialogPackageManager::SortPackages()
{
	ranges::sort(packages_, [](const auto& a, const auto& b)
		{
			return (StrCmpLogicalW(a.displayedName.c_str(), b.displayedName.c_str()) < 0);
		});
}

void CDialogPackageManager::UpdateListBoxFromData()
{
	SortPackages();

	const auto it = ranges::find_if(packages_, [&](const auto& package) { return (focusedPackageId_ == package.id); });
	if (it == packages_.cend())
	{
		if (packages_.empty())
		{
			focusedPackageIdx_ = -1;
			focusedPackageId_.clear();
		}
		else
		{
			focusedPackageIdx_ = 0;
			focusedPackageId_ = packages_[0].id;
		}
	}
	else
	{
		focusedPackageIdx_ = ranges::distance(packages_.cbegin(), it);
	}

	packagesListBox_.ResetContent();

	for (const auto& package: packages_)
	{
		const auto prefix = [&]() -> std::wstring {
			switch (package.status)
			{
			case config::PackageDelayStatus::ToBeRemoved:
				return L"(will be removed) ";
			case config::PackageDelayStatus::ToBeUpdated:
				return L"(will be updated) ";
			default:
				return L"";
			}
		}();
		packagesListBox_.AddString((prefix + package.displayedName).c_str());
	}
}

void CDialogPackageManager::UpdatedUiPackageInfo()
{
	if (focusedPackageIdx_ < 0)
	{
		packageInfoEdit_.SetWindowText(L"");
		return;
	}

	const auto packageData = packages_[focusedPackageIdx_];

	packageInfoEdit_.SetWindowText(L"");

	CHARFORMAT savedCharFormat{};
	packageInfoEdit_.GetSelectionCharFormat(savedCharFormat);
	auto autoFormat = wil::scope_exit([&] { packageInfoEdit_.SetSelectionCharFormat(savedCharFormat); });

	if (!packageData.parsedSettings)
	{
		CHARFORMAT newCharFormat = savedCharFormat;
		newCharFormat.dwMask = CFM_COLOR;
		newCharFormat.dwEffects = ~CFE_AUTOCOLOR;
		newCharFormat.crTextColor = RGB(255, 0, 0);

		packageInfoEdit_.SetSelectionCharFormat(newCharFormat);
		packageInfoEdit_.AppendText(L"Error:\r\n", FALSE);

		packageInfoEdit_.SetSelectionCharFormat(savedCharFormat);
		packageInfoEdit_.AppendText(packages_[focusedPackageIdx_].errorText.c_str(), FALSE);
	}
	else
	{
		const auto valueOrEmpty = [](const std::string& str) -> std::wstring {
			return (str.empty() ? L"<empty>" : qwr::ToWide(str));
		};

		const auto& parsedSettings = *packageData.parsedSettings;

		CHARFORMAT newCharFormat = savedCharFormat;
		newCharFormat.dwMask = CFM_UNDERLINE;
		newCharFormat.dwEffects = CFM_UNDERLINE;

		const auto appendText = [&](const wchar_t* field, const wchar_t* value) {
			assert(field);
			assert(value);

			packageInfoEdit_.SetSelectionCharFormat(newCharFormat);
			packageInfoEdit_.AppendText(field, FALSE);
			packageInfoEdit_.SetSelectionCharFormat(savedCharFormat);
			packageInfoEdit_.AppendText(L": ", FALSE);
			packageInfoEdit_.AppendText(value, FALSE);
		};

		appendText(L"Name", valueOrEmpty(parsedSettings.scriptName).c_str());
		packageInfoEdit_.AppendText(L"\r\n", FALSE);

		appendText(L"Version", valueOrEmpty(parsedSettings.scriptVersion).c_str());
		packageInfoEdit_.AppendText(L"\r\n", FALSE);

		appendText(L"Author", valueOrEmpty(parsedSettings.scriptAuthor).c_str());
		packageInfoEdit_.AppendText(L"\r\n", FALSE);

		appendText(L"Description", (L"\r\n" + valueOrEmpty(parsedSettings.scriptDescription)).c_str());
	}
}

CDialogPackageManager::PackageData CDialogPackageManager::GeneratePackageData(const config::ParsedPanelSettings& parsedSettings)
{
	const auto displayedName = [&parsedSettings] {
		return (parsedSettings.scriptAuthor.empty()
			? parsedSettings.scriptName
			: fmt::format("{} (by {})", parsedSettings.scriptName, parsedSettings.scriptAuthor));
	}();

	return PackageData{
		qwr::ToWide(displayedName),
		*parsedSettings.packageId,
		parsedSettings,
		L"",
		config::PackageDelayStatus::NotDelayed
	};
}

bool CDialogPackageManager::ImportPackage(const std::filesystem::path& path)
{
	assert(fs::exists(path));

	try
	{
		const auto tmpPath = path::TempFolder_PackageUnpack();
		fs::remove_all(tmpPath);
		fs::create_directories(tmpPath);
		auto autoTmp = wil::scope_exit([&] {
			try
			{
				fs::remove_all(tmpPath);
			}
			catch (const fs::filesystem_error&)
			{
			}
		});

		UnpackZip(path, tmpPath);

		auto newSettings = config::GetPackageSettingsFromPath(tmpPath);
		const auto& packageId = *newSettings.packageId;

		if (const auto oldPackagePathOpt = config::FindPackage(packageId); oldPackagePathOpt)
		{
			if (!ConfirmPackageOverwrite(*oldPackagePathOpt, newSettings))
			{
				return false;
			}

			if (config::IsPackageInUse(packageId))
			{
				config::MarkPackageAsToBeInstalled(packageId, tmpPath);

				auto it = ranges::find_if(packages_, [&](const auto& elem)
					{
						return (packageId == elem.id);
					});

				if (it != packages_.cend())
				{
					focusedPackageId_ = packageId;
					it->status = config::PackageDelayStatus::ToBeUpdated;
				}

				UpdateListBoxFromData();
				DoFullDdxToUi();
				return true;
			}

			fs::remove_all(*oldPackagePathOpt);
		}

		const auto newPackagePath = path::Packages_Profile() / packageId;
		fs::create_directories(newPackagePath);
		fs::copy(tmpPath, newPackagePath, fs::copy_options::recursive);

		newSettings.scriptPath = newPackagePath / config::GetRelativePathToMainFile();

		auto it = ranges::find_if(packages_, [&](const auto& elem)
			{
				return (packageId == elem.id);
			});

		if (it != packages_.cend())
		{
			*it = GeneratePackageData(newSettings);
		}
		else
		{
			packages_.emplace_back(GeneratePackageData(newSettings));
		}

		focusedPackageId_ = packageId;
		UpdateListBoxFromData();
		DoFullDdxToUi();
	}
	catch (const fs::filesystem_error& e)
	{
		qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, qwr::ToU8_FromAcpToWide(e.what()));
	}
	catch (const qwr::QwrException& e)
	{
		qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, e.what());
	}

	return false;
}

bool CDialogPackageManager::ConfirmPackageOverwrite(const std::filesystem::path& oldPackagePath, const config::ParsedPanelSettings& newSettings)
{
	try
	{
		const auto oldSettings = config::GetPackageSettingsFromPath(oldPackagePath);

		const int iRet = popup_message_v3::get()->messageBox(
			*this,
			fmt::format("Another version of this package is present:\nold: '{}' vs new: '{}'\n\nDo you want to update?",
				oldSettings.scriptVersion.empty() ? "<none>" : oldSettings.scriptVersion,
				newSettings.scriptVersion.empty() ? "<none>" : newSettings.scriptVersion
			).c_str(),
			"Importing package",
			MB_YESNO
		);

		if (iRet != IDYES)
		{
			return false;
		}

		if (oldSettings.scriptName != newSettings.scriptName)
		{
			const int iRet = popup_message_v3::get()->messageBox(
				*this,
				fmt::format("Currently installed package has a different name from the new one:\nold: '{}' vs new: '{}'\n\nDo you want to continue?",
					oldSettings.scriptName.empty() ? "<none>" : oldSettings.scriptName,
					newSettings.scriptName.empty() ? "<none>" : newSettings.scriptName
				).c_str(),
				"Importing package",
				MB_YESNO | MB_ICONWARNING
			);

			if (iRet != IDYES)
			{
				return false;
			}
		}
	}
	catch (const qwr::QwrException&)
	{
		// old package might be broken and unparseable,
		// but we still need to confirm
		const int iRet = popup_message_v3::get()->messageBox(
			*this,
			"Another version of this package is present.\n"
			"Do you want to update?",
			"Importing package",
			MB_YESNO
		);

		if (iRet != IDYES)
		{
			return false;
		}
	}

	return true;
}

bool CDialogPackageManager::ConfirmRebootOnPackageInUse()
{
	const int iRet = popup_message_v3::get()->messageBox(
		*this,
		"The package is currently in use. Changes will be applied on the next foobar2000 start.\n"
		"Do you want to restart foobar2000 now?",
		"Changing package",
		MB_YESNO);
	return (iRet == IDYES);
}

} // namespace smp::ui
