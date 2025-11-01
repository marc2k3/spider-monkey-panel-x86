#include <stdafx.h>
#include <pfc/string-conv-lite.h>

#include "unicode.h"

namespace qwr
{
	std::string ToU8(std::wstring_view src)
	{
		return pfc::utf8FromWide(src.data(), src.length()).get_ptr();
	}

	std::wstring ToWide(std::string_view str)
	{
		return pfc::wideFromUTF8(str.data(), str.length()).c_str();
	}

	std::wstring ToWide(const pfc::string_base& src)
	{
		return pfc::wideFromUTF8(src, src.length()).c_str();
	}

	std::string FS_Error_ToU8(const std::filesystem::filesystem_error& e)
	{
		const std::wstring msg = qwr::ToWide(e.code().message());
		std::wstring ret = fmt::format(L"{} \"{}\"", msg, e.path1().native());

		if (!e.path2().empty())
			ret += fmt::format(L", \"{}\"", e.path2().native());

		return ToU8(ret);
	}
}
