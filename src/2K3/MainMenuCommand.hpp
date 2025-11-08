#pragma once

class MainMenuCommand
{
public:
	MainMenuCommand(std::string_view command);

	static std::string build_parent_path(GUID parent) noexcept;

	bool execute() noexcept;

private:
	using Map = pfc::map_t<GUID, mainmenu_group::ptr>;

	static Map get_group_guid_map() noexcept;
	static bool is_disabled(const mainmenu_commands::ptr& ptr, uint32_t index) noexcept;

	bool execute_recur(mainmenu_node::ptr node, std::string_view parent_path) noexcept;

	std::string m_command;
};
