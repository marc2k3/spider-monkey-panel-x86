#pragma once

namespace qwr
{
	std::wstring ToWide(const char*) = delete;
	std::string ToU8(const wchar_t*) = delete;

	std::string ToU8(std::wstring_view src);
	std::wstring ToWide(std::string_view src);
	std::wstring ToWide(const pfc::string_base& src);

	std::string FS_Error_ToU8(const std::filesystem::filesystem_error& e);
}
