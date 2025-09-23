#include <stdafx.h>

#include "ui_conf_tab_properties.h"

#include <panel/js_panel_window.h>
#include <ui/ui_conf.h>

#include <2K3/FileDialog.hpp>
#include <2K3/TextFile.hpp>
#include <qwr/error_popup.h>
#include <qwr/type_traits.h>

namespace fs = std::filesystem;

namespace smp::ui
{

CConfigTabProperties::CConfigTabProperties(CDialogConf& parent, config::PanelProperties& properties)
	: parent_(parent)
	, properties_(properties)
{
}

HWND CConfigTabProperties::CreateTab(HWND hParent)
{
	return Create(hParent);
}

ATL::CDialogImplBase& CConfigTabProperties::Dialog()
{
	return *this;
}

const wchar_t* CConfigTabProperties::Name() const
{
	return L"Properties";
}

bool CConfigTabProperties::HasChanged()
{
	return false;
}

void CConfigTabProperties::Apply()
{
}

void CConfigTabProperties::Revert()
{
}

void CConfigTabProperties::Refresh()
{
	if (m_hWnd)
	{ // might be called while tab is inactive
		UpdateUiFromData();
	}
}

LRESULT CConfigTabProperties::OnInitDialog(HWND, LPARAM)
{
	DlgResize_Init(false, false, WS_CHILD);

	// Subclassing
	propertyListCtrl_.SubclassWindow(GetDlgItem(IDC_LIST_PROPERTIES));
	propertyListCtrl_.ModifyStyle(0, LBS_SORT | LBS_HASSTRINGS);
	propertyListCtrl_.SetExtendedListStyle(PLS_EX_SORTED | PLS_EX_XPLOOK);

	CWindow{ GetDlgItem(IDC_DEL) }.EnableWindow(propertyListCtrl_.GetCurSel() != -1);

	UpdateUiFromData();

	return TRUE; // set focus to default control
}

LRESULT CConfigTabProperties::OnPinItemChanged(LPNMHDR pnmh)
{
	auto pnpi = (LPNMPROPERTYITEM)pnmh;

	const auto hasChanged = [pnpi, &properties = properties_]() {
		auto& propValues = properties.values;

		if (!propValues.contains(pnpi->prop->GetName()))
		{
			return false;
		}

		auto& val = *propValues.at(pnpi->prop->GetName());
		_variant_t var;

		if (!pnpi->prop->GetValue(&var))
		{
			return false;
		}

		return std::visit([&var](auto& arg) {
			using T = std::decay_t<decltype(arg)>;
			const auto prevArgValue = arg;
			if constexpr (std::is_same_v<T, bool>)
			{
				var.ChangeType(VT_BOOL);
				arg = static_cast<bool>(var.boolVal);
			}
			else if constexpr (std::is_same_v<T, int32_t>)
			{
				var.ChangeType(VT_I4);
				arg = static_cast<int32_t>(var.lVal);
			}
			else if constexpr (std::is_same_v<T, double>)
			{
				if (VT_BSTR == var.vt)
				{
					arg = std::stod(var.bstrVal);
				}
				else
				{
					var.ChangeType(VT_R8);
					arg = var.dblVal;
				}
			}
			else if constexpr (std::is_same_v<T, std::string>)
			{
				var.ChangeType(VT_BSTR);
				arg = qwr::ToU8(std::wstring_view{ var.bstrVal ? var.bstrVal : L"" });
			}
			else
			{
				static_assert(qwr::always_false_v<T>, "non-exhaustive visitor!");
			}

			return (prevArgValue != arg);
		}, val);
	}();

	if (hasChanged)
	{
		parent_.OnDataChanged();
	}

	return 0;
}

LRESULT CConfigTabProperties::OnSelChanged(LPNMHDR)
{
	UpdateUiDelButton();
	return 0;
}

LRESULT CConfigTabProperties::OnClearAllBnClicked(WORD, WORD, HWND)
{
	properties_.values.clear();

	propertyListCtrl_.ResetContent();

	parent_.OnDataChanged();

	return 0;
}

LRESULT CConfigTabProperties::OnDelBnClicked(WORD, WORD, HWND)
{
	if (int idx = propertyListCtrl_.GetCurSel(); idx >= 0)
	{
		HPROPERTY hproperty = propertyListCtrl_.GetProperty(idx);
		std::wstring name = hproperty->GetName();

		properties_.values.erase(name);

		propertyListCtrl_.DeleteItem(hproperty);
	}

	UpdateUiDelButton();
	parent_.OnDataChanged();

	return 0;
}

LRESULT CConfigTabProperties::OnImportBnClicked(WORD, WORD, HWND)
{
	auto path_func = [this](fb2k::stringRef path)
		{
			const auto native = filesystem::g_get_native_path(path->c_str());
			const auto wpath = qwr::ToWide(native);
			const auto str = TextFile(wpath).read();

			try
			{
				properties_ = smp::config::PanelProperties::FromJson(str);
				UpdateUiFromData();
			}
			catch (const qwr::QwrException& e)
			{
				qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, e.what());
			}
			catch (const pfc::exception& e)
			{
				qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, e.what());
			}

			parent_.OnDataChanged();
		};

	FileDialog::open(m_hWnd, "Import from", "Property files|*.json|All files|*.*", path_func);
	return 0;
}

LRESULT CConfigTabProperties::OnExportBnClicked(WORD, WORD, HWND)
{
	auto path_func = [this](fb2k::stringRef path)
		{
			const auto native = filesystem::g_get_native_path(path->c_str());
			const auto wpath = qwr::ToWide(native);
			TextFile(wpath).write(properties_.ToJson());
		};

	FileDialog::save(m_hWnd, "Import from", "Property files|*.json|All files|*.*", "json", path_func);
	return 0;
}

void CConfigTabProperties::UpdateUiFromData()
{
	propertyListCtrl_.ResetContent();

	struct LowerLexCmp
	{ // lexicographical comparison but with lower cased chars
		bool operator()(const std::wstring& a, const std::wstring& b) const
		{
			return (_wcsicmp(a.c_str(), b.c_str()) < 0);
		}
	};
	std::map<std::wstring, HPROPERTY, LowerLexCmp> propMap;
	for (const auto& [name, pSerializedValue]: properties_.values)
	{
		HPROPERTY hProp = std::visit([&name = name](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int32_t>)
			{
				return PropCreateSimple(name.c_str(), arg);
			}
			else if constexpr (std::is_same_v<T, double>)
			{
				const std::wstring strNumber = [arg] {
					if (std::trunc(arg) == arg)
					{ // Most likely uint64_t
						return std::to_wstring(static_cast<uint64_t>(arg));
					}

					// std::to_string(double) has precision of float
					return fmt::format(L"{:.16g}", arg);
				}();
				return PropCreateSimple(name.c_str(), strNumber.c_str());
			}
			else if constexpr (std::is_same_v<T, std::string>)
			{
				return PropCreateSimple(name.c_str(), qwr::ToWide(arg).c_str());
			}
			else
			{
				static_assert(qwr::always_false_v<T>, "non-exhaustive visitor!");
			}
		}, *pSerializedValue);

		propMap.emplace(name, hProp);
	}

	for (auto& [name, hProp]: propMap)
	{
		propertyListCtrl_.AddItem(hProp);
	}
}

void CConfigTabProperties::UpdateUiDelButton()
{
	CWindow{ GetDlgItem(IDC_DEL) }.EnableWindow(propertyListCtrl_.GetCurSel() != -1);
}

} // namespace smp::ui
