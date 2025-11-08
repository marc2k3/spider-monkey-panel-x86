#pragma once

namespace smp::utils
{
	bool DoesPathMatchCommand(std::string_view path, std::string_view command);

	/// @throw qwr::QwrException
	uint32_t GetMainmenuCommandStatusByName(const std::string& name);
}
