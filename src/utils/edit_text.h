#pragma once

namespace smp
{
	/// @throw qwr::QwrException
	void EditTextFile(HWND hParent, const std::filesystem::path& file, bool isPanelScript, bool isModal);

	/// @throw qwr::QwrException
	void EditText(HWND hParent, std::string& text, bool isPanelScript);
}
