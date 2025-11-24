#pragma once
#include <config/parsed_panel_config.h>

namespace smp::panel
{
	/// @throw QwrException
	void EditScript(HWND hParent, config::ParsedPanelSettings& settings);

	/// @throw QwrException
	void EditPackageScript(HWND hParent, const std::filesystem::path& script, const config::ParsedPanelSettings& settings);
}
