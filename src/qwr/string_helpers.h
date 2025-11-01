#pragma once

namespace qwr::string
{
	static std::vector<std::string_view> SplitByLines(std::string_view str)
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

	template <typename T>
	std::vector<std::basic_string_view<T>> Split(std::basic_string_view<T> str, const std::basic_string<T>& separator)
	{
		return ranges::views::split(str, separator) | ranges::views::transform([](auto&& rng)
			{
				return std::basic_string_view<T>{ &*rng.begin(), static_cast<size_t>(ranges::distance(rng)) };
			}) | ranges::to_vector;
	}

	template <typename T>
	std::vector<std::basic_string_view<T>> Split(std::basic_string_view<T> str, T separator)
	{
		return Split(str, std::basic_string<T>(1, separator));
	}

	template <typename T>
	std::optional<T> GetNumber(std::string_view strView, int base = 10)
	{
		T number{};

		if (auto [pos, ec] = std::from_chars(strView.data(), strView.data() + strView.size(), number, base); ec == std::errc{})
		{
			return number;
		}
		else
		{
			return std::nullopt;
		}
	}
}
