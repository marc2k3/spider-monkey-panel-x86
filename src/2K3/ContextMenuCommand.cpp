#include <stdafx.h>
#include "ContextMenuCommand.hpp"

#include <utils/menu_helpers.h>

ContextMenuCommand::ContextMenuCommand(std::string_view command, uint32_t flags) : m_command(command)
{
	if (playback_control::get()->is_playing())
	{
		m_cm = contextmenu_manager::get();
		m_cm->init_context_now_playing(flags);
	}
}

ContextMenuCommand::ContextMenuCommand(std::string_view command, metadb_handle_list_cref handles, uint32_t flags) : m_command(command)
{
	if (handles.get_count() > 0uz)
	{
		m_cm = contextmenu_manager::get();
		m_cm->init_context(handles, flags);
	}
}

bool ContextMenuCommand::execute() noexcept
{
	if (m_cm.is_empty())
		return false;

	return execute_recur(m_cm->get_root());
}

bool ContextMenuCommand::execute_recur(contextmenu_node* parent, std::string_view parent_path) noexcept
{
	for (const auto i : ranges::views::indices(parent->get_num_children()))
	{
		const auto child = parent->get_child(i);
		const auto type = child->get_type();

		if (type == contextmenu_item_node::type_separator)
			continue;

		const auto path = fmt::format("{}{}", parent_path, child->get_name());

		if (type == contextmenu_item_node::type_group)
		{
			if (execute_recur(child, path + "/"))
				return true;
		}
		else if (type == contextmenu_item_node::type_command && smp::utils::DoesPathMatchCommand(path, m_command))
		{
			if (WI_IsAnyFlagSet(child->get_display_flags(), contextmenu_item_node::FLAG_DISABLED_GRAYED))
				return false;

			child->execute();
			return true;
		}
	}

	return false;
}
