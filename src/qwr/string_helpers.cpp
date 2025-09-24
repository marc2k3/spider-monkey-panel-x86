#include <stdafx.h>
#include "string_helpers.h"

namespace qwr::string
{

std::vector<std::string_view> SplitByLines(std::string_view str)
{
	std::vector<std::string_view> lines;
	for (std::string_view curScope = str; !curScope.empty();)
	{
		if (size_t pos = curScope.find_first_of("\r\n"); std::string::npos != pos)
		{
			if (pos)
			{
				lines.emplace_back(curScope.data(), pos);
				curScope.remove_prefix(pos);
			}

			while (!curScope.empty() && (curScope[0] == '\r' || curScope[0] == '\n'))
			{
				curScope.remove_prefix(1);
			}
		}
		else
		{
			lines.emplace_back(curScope.data(), curScope.size());
			curScope = std::string_view{};
		}
	}

	return lines;
}

} // namespace qwr::string
