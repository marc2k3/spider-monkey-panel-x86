#pragma once

namespace smp
{

class DynamicMainMenuManager
{
public:
	struct CommandData
	{
		std::string name;
		std::optional<std::string> description;
	};

	struct PanelData
	{
		std::string name;
		std::unordered_map<uint32_t, CommandData> commands;
	};

public:
	static [[nodiscard]] DynamicMainMenuManager& Get();

	void RegisterPanel(HWND hWnd, const std::string& panelName);
	void UnregisterPanel(HWND hWnd);

	/// @throw QwrException
	void RegisterCommand(HWND hWnd, uint32_t id, const std::string& name, const std::optional<std::string>& description);
	/// @throw QwrException
	void UnregisterCommand(HWND hWnd, uint32_t id);

	const std::unordered_map<HWND, PanelData>& GetAllCommandData() const;

private:
	DynamicMainMenuManager() = default;

private:
	std::unordered_map<HWND, PanelData> panels_;
};

} // namespace smp
