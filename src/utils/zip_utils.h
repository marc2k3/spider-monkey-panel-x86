#pragma once
#include <miniz/miniz.h>

namespace smp
{
	class ZipPacker
	{
	public:
		/// @throw SmpException
		[[nodiscard]] ZipPacker(const std::filesystem::path& zipFile);

		/// @remark Deletes zip file on error
		~ZipPacker();

		/// @throw SmpException
		void AddFile(const std::filesystem::path& srcFile, const std::string& destFileName);
		/// @throw SmpException
		void AddFolder(const std::filesystem::path& srcFolder, const std::string& destFolderName = "");
		/// @throw SmpException
		void Finish();

	private:
		std::unique_ptr<mz_zip_archive> pZip_;
		std::filesystem::path zipFile_;
	};

	/// @throw SmpException
	void UnpackZip(const std::filesystem::path& zipFile, const std::filesystem::path& dstFolder);
}
