#pragma once
#include <config/parsed_panel_config.h>

namespace smp::config
{
	// TODO: cleanup methods and their naming

	[[nodiscard]] const std::filesystem::path& GetRelativePathToMainFile();

	/// @throw QwrException
	[[nodiscard]] std::optional<std::filesystem::path> FindPackage(const std::string& packageId);

	/// @throw QwrException
	[[nodiscard]] ParsedPanelSettings GetNewPackageSettings(const std::string& name);

	/// @throw QwrException
	[[nodiscard]] ParsedPanelSettings GetPackageSettingsFromPath(const std::filesystem::path& packagePath);

	/// @throw QwrException
	void FillPackageSettingsFromPath(const std::filesystem::path& packagePath, ParsedPanelSettings& settings);

	/// @throw QwrException
	void MaybeSavePackageData(const ParsedPanelSettings& settings);

	/// @throw QwrException
	[[nodiscard]] std::filesystem::path GetPackageScriptsDir(const ParsedPanelSettings& settings);

	/// @throw QwrException
	[[nodiscard]] std::filesystem::path GetPackageAssetsDir(const ParsedPanelSettings& settings);

	/// @throw QwrException
	[[nodiscard]] std::filesystem::path GetPackageStorageDir(const ParsedPanelSettings& settings);


	[[nodiscard]] std::filesystem::path GetPackagePath(const ParsedPanelSettings& settings);
	[[nodiscard]] WStrings GetPackageFiles(const ParsedPanelSettings& settings);
	[[nodiscard]] WStrings GetPackageScriptFiles(const ParsedPanelSettings& settings);
}
