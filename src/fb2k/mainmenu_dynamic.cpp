#include <stdafx.h>

#include "mainmenu_dynamic.h"

#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>

using namespace smp;

namespace
{

class MainMenuNodeCommand_PanelCommand : public mainmenu_node_command
{
public:
	MainMenuNodeCommand_PanelCommand(HWND panelHwnd,
									const std::string& panelName,
									uint32_t commandId,
									const std::string& commandName,
									const std::optional<std::string>& commandDescription);

	void get_display(pfc::string_base& text, uint32_t& flags) override;
	void execute(service_ptr_t<service_base> callback) override;
	GUID get_guid() override;
	bool get_description(pfc::string_base& out) override;

private:
	const HWND panelHwnd_;
	const std::string panelName_;
	const uint32_t commandId_;
	const std::string commandName_;
	const std::optional<std::string> commandDescriptionOpt_;
};

class MainMenuNodeGroup_PanelCommands : public mainmenu_node_group
{
public:
	MainMenuNodeGroup_PanelCommands(HWND panelHwnd,
									const std::string& panelName,
									const std::unordered_map<uint32_t, DynamicMainMenuManager::CommandData>& idToCommand);
	void get_display(pfc::string_base& text, uint32_t& flags) override;
	t_size get_children_count() override;
	mainmenu_node::ptr get_child(t_size index) override;

private:
	const smp::DynamicMainMenuManager::PanelData panelData_;
	const std::string panelName_;
	std::vector<mainmenu_node::ptr> commandNodes_;
};

class MainMenuNodeGroup_Panels : public mainmenu_node_group
{
public:
	MainMenuNodeGroup_Panels();
	void get_display(pfc::string_base& text, uint32_t& flags) override;
	t_size get_children_count() override;
	mainmenu_node::ptr get_child(t_size index) override;

private:
	std::vector<mainmenu_node::ptr> panelNodes_;
};

class MainMenuCommands_Panels : public mainmenu_commands_v2
{
public:
	// mainmenu_commands
	uint32_t get_command_count() override;
	GUID get_command(uint32_t p_index) override;
	void get_name(uint32_t p_index, pfc::string_base& p_out) override;
	bool get_description(uint32_t p_index, pfc::string_base& p_out) override;
	GUID get_parent() override;
	void execute(uint32_t p_index, service_ptr_t<service_base> p_callback) override;

	// mainmenu_commands_v2
	bool is_command_dynamic(uint32_t index) override;
	mainmenu_node::ptr dynamic_instantiate(uint32_t index) override;
	bool dynamic_execute(uint32_t index, const GUID& subID, service_ptr_t<service_base> callback) override;
};

} // namespace

namespace
{

MainMenuNodeCommand_PanelCommand::MainMenuNodeCommand_PanelCommand(
	HWND panelHwnd,
	const std::string& panelName,
	uint32_t commandId,
	const std::string& commandName,
	const std::optional<std::string>& commandDescription)
	: panelHwnd_(panelHwnd)
	, panelName_(panelName)
	, commandId_(commandId)
	, commandName_(commandName)
	, commandDescriptionOpt_(commandDescription)
{
}

void MainMenuNodeCommand_PanelCommand::get_display(pfc::string_base& text, uint32_t& flags)
{
	text.clear();
	text << commandName_;
	flags = 0;
}

void MainMenuNodeCommand_PanelCommand::execute(service_ptr_t<service_base> callback)
{
	EventDispatcher::Get().PutEvent(panelHwnd_, GenerateEvent_JsCallback(EventId::kDynamicMainMenu, commandId_));
}

GUID MainMenuNodeCommand_PanelCommand::get_guid()
{
	auto api = hasher_md5::get();
	hasher_md5_state state;
	api->initialize(state);
	// process termination character as well - it will act as a separator
	api->process(state, panelName_.data(), panelName_.size() + 1);
	api->process(state, &commandId_, sizeof(commandId_));

	return api->get_result_guid(state);
}

bool MainMenuNodeCommand_PanelCommand::get_description(pfc::string_base& out)
{
	if (!commandDescriptionOpt_)
	{
		return false;
	}

	out.clear();
	out << *commandDescriptionOpt_;
	return true;
}

MainMenuNodeGroup_PanelCommands::MainMenuNodeGroup_PanelCommands(
	HWND panelHwnd,
	const std::string& panelName,
	const std::unordered_map<uint32_t, DynamicMainMenuManager::CommandData>& idToCommand)
	: panelName_(panelName)
{
	// use map to sort commands by their name
	const auto commandNameToId =
		idToCommand
		| ranges::views::transform([](const auto& elem) { return std::make_pair(elem.second.name, elem.first); })
		| ranges::to<std::multimap>;
	for (const auto& [name, id]: commandNameToId)
	{
		const auto& command = idToCommand.at(id);
		commandNodes_.emplace_back(fb2k::service_new<MainMenuNodeCommand_PanelCommand>(panelHwnd, panelName_, id, command.name, command.description));
	}
}

void MainMenuNodeGroup_PanelCommands::get_display(pfc::string_base& text, uint32_t& flags)
{
	text.clear();
	text << panelName_;
	flags = mainmenu_commands::flag_defaulthidden | mainmenu_commands::sort_priority_base;
}

t_size MainMenuNodeGroup_PanelCommands::get_children_count()
{
	return commandNodes_.size();
}

mainmenu_node::ptr MainMenuNodeGroup_PanelCommands::get_child(t_size index)
{
	assert(index < commandNodes_.size());
	return commandNodes_.at(index);
}

MainMenuNodeGroup_Panels::MainMenuNodeGroup_Panels()
{
	const auto panels = smp::DynamicMainMenuManager::Get().GetAllCommandData();
	// use map to sort panels by their name
	const auto panelNameToHWnd =
		panels
		| ranges::views::transform([](const auto& elem) { return std::make_pair(elem.second.name, elem.first); })
		| ranges::to<std::map>;
	for (const auto& [name, hWnd]: panelNameToHWnd)
	{
		const auto& panelData = panels.at(hWnd);
		if (panelData.commands.empty())
		{
			continue;
		}

		panelNodes_.emplace_back(fb2k::service_new<MainMenuNodeGroup_PanelCommands>(hWnd, panelData.name, panelData.commands));
	}
}

void MainMenuNodeGroup_Panels::get_display(pfc::string_base& text, uint32_t& flags)
{
	text = "Script commands";
	flags = mainmenu_commands::flag_defaulthidden | mainmenu_commands::sort_priority_base;
}

t_size MainMenuNodeGroup_Panels::get_children_count()
{
	return panelNodes_.size();
}

mainmenu_node::ptr MainMenuNodeGroup_Panels::get_child(t_size index)
{
	assert(index < panelNodes_.size());
	return panelNodes_.at(index);
}

uint32_t MainMenuCommands_Panels::get_command_count()
{
	return 1;
}

GUID MainMenuCommands_Panels::get_command(uint32_t /*p_index*/)
{
	return smp::guid::menu_script_commands;
}

void MainMenuCommands_Panels::get_name(uint32_t /*p_index*/, pfc::string_base& p_out)
{
	p_out = "Script commands";
}

bool MainMenuCommands_Panels::get_description(uint32_t /*p_index*/, pfc::string_base& p_out)
{
	p_out = "Commands provided by scripts.";
	return true;
}

GUID MainMenuCommands_Panels::get_parent()
{
	return smp::guid::mainmenu_group_predefined;
}

void MainMenuCommands_Panels::execute(uint32_t /*p_index*/, service_ptr_t<service_base> /*p_callback*/)
{
	// Should not get here, someone not aware of our dynamic status tried to invoke us?
}

bool MainMenuCommands_Panels::is_command_dynamic(uint32_t /*index*/)
{
	return true;
}

mainmenu_node::ptr MainMenuCommands_Panels::dynamic_instantiate(uint32_t /*index*/)
{
	return fb2k::service_new<MainMenuNodeGroup_Panels>();
}

bool MainMenuCommands_Panels::dynamic_execute(uint32_t index, const GUID& subID, service_ptr_t<service_base> callback)
{
	return __super::dynamic_execute(index, subID, callback);
}
} // namespace

namespace
{

FB2K_SERVICE_FACTORY(MainMenuCommands_Panels);

} // namespace

namespace smp
{

DynamicMainMenuManager& DynamicMainMenuManager::Get()
{
	static DynamicMainMenuManager dmmm;
	return dmmm;
}

void DynamicMainMenuManager::RegisterPanel(HWND hWnd, const std::string& panelName)
{
	assert(!panels_.contains(hWnd));
	panels_.try_emplace(hWnd, PanelData{ panelName });
}

void DynamicMainMenuManager::UnregisterPanel(HWND hWnd)
{
	// don't check hWnd presence, since this might be called several times during error handling
	panels_.erase(hWnd);
}

void DynamicMainMenuManager::RegisterCommand(HWND hWnd, uint32_t id, const std::string& name, const std::optional<std::string>& description)
{
	assert(panels_.contains(hWnd));

	auto& panelData = panels_.at(hWnd);
	QwrException::ExpectTrue(!panelData.commands.contains(id), "Command with id `{}` was already registered", id);

	panelData.commands.try_emplace(id, CommandData{ name, description });
}

void DynamicMainMenuManager::UnregisterCommand(HWND hWnd, uint32_t id)
{
	assert(panels_.contains(hWnd));

	auto& panelData = panels_.at(hWnd);
	QwrException::ExpectTrue(panelData.commands.contains(id), "Unknown command id `{}`", id);

	panelData.commands.erase(id);
}

const std::unordered_map<HWND, DynamicMainMenuManager::PanelData>&
DynamicMainMenuManager::GetAllCommandData() const
{
	return panels_;
}

} // namespace smp
