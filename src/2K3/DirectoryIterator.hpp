#pragma once

class DirectoryIterator
{
public:
	DirectoryIterator(const std::filesystem::path& directory) : m_directory(directory) {}

	WStrings list_files(bool recur = false) noexcept
	{
		if (recur)
			return list_t<std::filesystem::recursive_directory_iterator>(EntryType::File);

		return list_t(EntryType::File);
	}

	WStrings list_directories(bool recur = false) noexcept
	{
		if (recur)
			return list_t<std::filesystem::recursive_directory_iterator>(EntryType::Directory);

		return list_t(EntryType::Directory);
	}

private:
	enum class EntryType
	{
		File,
		Directory,
	};

	template <typename FSIterator = std::filesystem::directory_iterator>
	WStrings list_t(EntryType type) noexcept
	{
		WStrings paths;
		std::error_code ec;

		if (std::filesystem::is_directory(m_directory, ec))
		{
			for (const std::filesystem::directory_entry& entry : FSIterator(m_directory, std::filesystem::directory_options::skip_permission_denied))
			{
				if (type == EntryType::File && entry.is_regular_file())
				{
					paths.emplace_back(entry.path().native());
				}
				else if (type == EntryType::Directory && entry.is_directory())
				{
					paths.emplace_back(entry.path().native() + std::filesystem::path::preferred_separator);
				}
			}

			ranges::sort(paths, CmpW());
		}

		return paths;
	}

	std::filesystem::path m_directory;
};
