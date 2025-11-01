#pragma once

namespace qwr
{

class QwrException
	: public std::runtime_error
{
public:
	template <typename... Args>
	explicit QwrException(std::string_view errorMessage, Args&&... errorMessageFmtArgs)
		: std::runtime_error(fmt::format(fmt::runtime(errorMessage), std::forward<Args>(errorMessageFmtArgs)...))
	{
	}

	template <typename... Args>
	explicit QwrException(std::wstring_view errorMessage, Args&&... errorMessageFmtArgs)
		: std::runtime_error(qwr::ToU8(
			fmt::format(fmt::runtime(errorMessage), std::forward<Args>(errorMessageFmtArgs)...)))
	{
	}

	explicit QwrException(const std::filesystem::filesystem_error& e)
		: std::runtime_error(qwr::FS_Error_ToU8(e))
	{
	}

	~QwrException() override = default;

	template <typename StrT, typename... Args>
	_Post_satisfies_(checkValue) static void ExpectTrue(bool checkValue, StrT errorMessage, Args&&... errorMessageFmtArgs)
	{
		if (!checkValue)
		{
			throw QwrException(errorMessage, std::forward<Args>(errorMessageFmtArgs)...);
		}
	}

	/// @details This overload is needed for SAL: it can't understand that `(bool)ptr == true` is the same as  `ptr != null`
	template <typename StrT, typename... Args>
	static void ExpectTrue(_Post_notnull_ void* checkValue, StrT errorMessage, Args&&... errorMessageFmtArgs)
	{
		ExpectTrue(static_cast<bool>(checkValue), errorMessage, std::forward<Args>(errorMessageFmtArgs)...);
	}
};

} // namespace qwr
