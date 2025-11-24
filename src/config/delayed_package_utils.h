#pragma once

namespace smp::config
{
	enum class PackageDelayStatus
	{
		ToBeRemoved,
		ToBeUpdated,
		NotDelayed
	};

	[[nodiscard]] bool IsPackageInUse(const std::string& packageId) noexcept;

	/// @throw QwrException
	[[nodiscard]] PackageDelayStatus GetPackageDelayStatus(const std::string& packageId);

	/// @throw QwrException
	void ClearPackageDelayStatus(const std::string& packageId);

	/// @throw QwrException
	void MarkPackageAsToBeRemoved(const std::string& packageId);

	/// @throw QwrException
	void MarkPackageAsToBeInstalled(const std::string& packageId, const std::filesystem::path& packageContent);

	/// @throw QwrException
	void MarkPackageAsInUse(const std::string& packageId);

	void ProcessDelayedPackages();
}
