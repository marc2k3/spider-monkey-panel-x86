#include <stdafx.h>
#include "MainMenuCommand.hpp"

#include <utils/menu_helpers.h>

MainMenuCommand::MainMenuCommand(std::string_view command) : m_command(command) {}

#pragma region static
MainMenuCommand::Map MainMenuCommand::get_group_guid_map() noexcept
{
	Map group_guid_map;

	for (auto ptr : mainmenu_group::enumerate())
	{
		group_guid_map.set(ptr->get_guid(), ptr);
	}

	return group_guid_map;
}

bool MainMenuCommand::is_disabled(const mainmenu_commands::ptr& ptr, uint32_t index) noexcept
{
	pfc::string8 display;
	uint32_t flags{};
	ptr->get_display(index, display, flags);

	return WI_IsFlagSet(flags, mainmenu_commands::flag_disabled);
}

std::string MainMenuCommand::build_parent_path(GUID parent) noexcept
{
	static const auto group_guid_map = get_group_guid_map();
	Strings strings;

	while (parent != pfc::guid_null)
	{
		const auto group_ptr = group_guid_map[parent];
		mainmenu_group_popup::ptr group_popup_ptr;

		if (group_ptr->cast(group_popup_ptr))
		{
			pfc::string8 str;
			group_popup_ptr->get_display_string(str);
			strings.emplace_back(str.get_ptr());
		}

		parent = group_ptr->get_parent();
	}

	return fmt::format("{}/", fmt::join(strings | std::views::reverse, "/"));
}
#pragma endregion

bool MainMenuCommand::execute() noexcept
{
	// Ensure commands on the Edit menu are enabled
	ui_edit_context_manager::get()->set_context_active_playlist();

	for (auto ptr : mainmenu_commands::enumerate())
	{
		mainmenu_commands_v2::ptr v2_ptr;
		ptr->cast(v2_ptr);

		const auto parent_path = build_parent_path(ptr->get_parent());

		for (const auto i : ranges::views::indices(ptr->get_command_count()))
		{
			if (v2_ptr.is_valid() && v2_ptr->is_command_dynamic(i))
			{
				const auto node = v2_ptr->dynamic_instantiate(i);

				if (execute_recur(node, parent_path))
					return true;
			}
			else
			{
				pfc::string8 name;
				ptr->get_name(i, name);
				const auto path = fmt::format("{}{}", parent_path, name.get_ptr());

				if (smp::utils::DoesPathMatchCommand(path, m_command))
				{
					if (is_disabled(ptr, i))
						return false;

					ptr->execute(i, nullptr);
					return true;
				}
			}
		}
	}

	return false;
}

bool MainMenuCommand::execute_recur(mainmenu_node::ptr node, std::string_view parent_path) noexcept
{
	const auto type = node->get_type();

	if (type == mainmenu_node::type_separator)
		return false;

	pfc::string8 text;
	uint32_t flags{};
	node->get_display(text, flags);
	auto path = fmt::format("{}{}", parent_path, text.get_ptr());

	if (type == mainmenu_node::type_group)
	{
		if (!path.ends_with("/"))
		{
			path.append("/");
		}

		for (const auto i : ranges::views::indices(node->get_children_count()))
		{
			const auto child = node->get_child(i);

			if (execute_recur(child, path))
				return true;
		}
	}
	else if (type == mainmenu_node::type_command && smp::utils::DoesPathMatchCommand(path, m_command))
	{
		if (WI_IsFlagSet(flags, mainmenu_commands::flag_disabled))
			return false;

		node->execute(nullptr);
		return true;
	}

	return false;
}

