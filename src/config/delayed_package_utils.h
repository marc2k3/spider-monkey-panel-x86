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

	/// @throw qwr::QwrException
	[[nodiscard]] PackageDelayStatus GetPackageDelayStatus(const std::string& packageId);

	/// @throw qwr::QwrException
	void ClearPackageDelayStatus(const std::string& packageId);

	/// @throw qwr::QwrException
	void MarkPackageAsToBeRemoved(const std::string& packageId);

	/// @throw qwr::QwrException
	void MarkPackageAsToBeInstalled(const std::string& packageId, const std::filesystem::path& packageContent);

	/// @throw qwr::QwrException
	void MarkPackageAsInUse(const std::string& packageId);

	void ProcessDelayedPackages();
}
