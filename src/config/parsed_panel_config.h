#pragma once
#include <config/panel_config.h>

namespace smp::config
{

enum class ScriptSourceType : uint8_t
{
	Package = 0,
	Sample,
	File,
	InMemory,
};

struct ParsedPanelSettings
{
	std::string panelId;
	std::optional<std::string> script;
	std::optional<std::filesystem::path> scriptPath;
	std::optional<std::string> packageId;
	std::string scriptName;
	std::string scriptVersion;
	std::string scriptAuthor;
	std::string scriptDescription;
	bool isSample = false;

	EdgeStyle edgeStyle = EdgeStyle::Default;
	bool isPseudoTransparent = false;
	bool shouldGrabFocus = true;
	bool enableDragDrop = false;

	[[nodiscard]] static ParsedPanelSettings GetDefault();

	/// @throw QwrException
	[[nodiscard]] static ParsedPanelSettings Parse(const PanelSettings& settings);
	/// @throw QwrException
	[[nodiscard]] static ParsedPanelSettings Reparse(const ParsedPanelSettings& parsedSettings);

	[[nodiscard]] PanelSettings GeneratePanelSettings() const;

	[[nodiscard]] ScriptSourceType GetSourceType() const;
};

} // namespace smp::config