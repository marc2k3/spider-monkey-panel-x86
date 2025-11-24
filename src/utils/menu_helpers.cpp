#include <stdafx.h>
#include "menu_helpers.h"

#include <2K3/MainMenuCommand.hpp>
#include <utils/guid_helpers.h>

namespace
{
	mainmenu_node::ptr FindMainmenuCommandV2NodeRecur(mainmenu_node::ptr node, const std::string& basePath, const std::string& name)
	{
		if (mainmenu_node::type_separator == node->get_type())
			return {};

		auto curPath = basePath;

		pfc::string8 displayName;
		uint32_t tmp;
		node->get_display(displayName, tmp);
		if (!displayName.is_empty())
		{
			curPath += displayName.c_str();
		}

		switch (node->get_type())
		{
		case mainmenu_node::type_command:
		{
			if (smp::utils::DoesPathMatchCommand(curPath, name))
			{
				return node;
			}
			break;
		}
		case mainmenu_node::type_group:
		{
			if (curPath.back() != '/')
			{
				curPath += '/';
			}

			for (auto i: ranges::views::indices(node->get_children_count()))
			{
				auto pChild = node->get_child(i);
				if (auto retVal = FindMainmenuCommandV2NodeRecur(pChild, curPath, name); retVal.is_valid())
				{
					return retVal;
				}
			}
			break;
		}
		default:
			break;
		}

		return {};
	}

	template <typename F_New, typename F_Old>
	bool ApplyFnOnMainmenuNode(const std::string& name, F_New fnNew, F_Old fnOld)
	{
		// Ensure commands on the Edit menu are enabled
		ui_edit_context_manager::get()->set_context_active_playlist();

		for (auto ptr : mainmenu_commands::enumerate())
		{
			mainmenu_commands_v2::ptr v2_ptr;
			ptr->cast(v2_ptr);

			const auto parent_path = MainMenuCommand::build_parent_path(ptr->get_parent());

			for (const auto idx: ranges::views::indices(ptr->get_command_count()))
			{
				if (v2_ptr.is_valid() && v2_ptr->is_command_dynamic(idx))
				{
					auto node = v2_ptr->dynamic_instantiate(idx);

					if (auto retVal = FindMainmenuCommandV2NodeRecur(node, parent_path, name); retVal.is_valid())
					{
						fnNew(retVal);
						return true;
					}
				}
				else
				{
					pfc::string8 command;
					ptr->get_name(idx, command);
					const auto path = fmt::format("{}{}", parent_path, command.get_ptr());

					if (smp::utils::DoesPathMatchCommand(path, name))
					{
						fnOld(idx, ptr);
						return true;
					}
				}
			}
		}

		return false;
	}
}

namespace smp::utils
{
	bool DoesPathMatchCommand(std::string_view path, std::string_view command)
	{
		const auto commandLen = command.length();
		const auto pathLen = path.length();

		if (commandLen > pathLen)
		{
			return false;
		}

		if (commandLen == pathLen)
		{
			return _stricmp(command.data(), path.data()) == 0;
		}

		return (path[pathLen - commandLen - 1] == '/') && _stricmp(path.data() + pathLen - commandLen, command.data()) == 0;
	}

	uint32_t GetMainmenuCommandStatusByName(const std::string& name)
	{
		uint32_t status{};

		const bool bRet = ApplyFnOnMainmenuNode(
			name,
			[&status](auto node) {
				pfc::string8 tmp;
				status = 0;
				node->get_display(tmp, status);
			},
			[&status](auto idx, auto ptr) {
				pfc::string8 tmp;
				status = 0;
				ptr->get_display(idx, tmp, status);
			});

		QwrException::ExpectTrue(bRet, "Unknown menu command: {}", name);
		return status;
	}
}
