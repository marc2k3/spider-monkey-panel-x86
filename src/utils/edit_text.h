#pragma once

namespace smp
{
	/// @throw QwrException
	void EditTextFile(HWND hParent, const std::filesystem::path& file, bool isPanelScript, bool isModal);

	/// @throw QwrException
	void EditText(HWND hParent, std::string& text, bool isPanelScript);
}
