#pragma once

class ContextMenuCommand
{
public:
	ContextMenuCommand(std::string_view command, uint32_t flags);
	ContextMenuCommand(std::string_view command, metadb_handle_list_cref handles, uint32_t flags);

	bool execute() noexcept;

private:
	bool execute_recur(contextmenu_node* parent, std::string_view parent_path = "") noexcept;

	contextmenu_manager::ptr m_cm;
	std::string m_command;
};
