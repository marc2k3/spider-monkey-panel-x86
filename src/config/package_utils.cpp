#include <stdafx.h>
#include "package_utils.h"

#include <2K3/DirectoryIterator.hpp>
#include <2K3/TextFile.hpp>
#include <utils/guid_helpers.h>

namespace fs = std::filesystem;

namespace
{
	using namespace smp;

	void Parse_PackageFromPath(const std::filesystem::path& packageDir, config::ParsedPanelSettings& parsedSettings)
	{
		try
		{
			qwr::QwrException::ExpectTrue(fs::exists(packageDir),
										"Can't find the required package: `{}`",
										packageDir.u8string());

			const auto packageJsonFile = packageDir / "package.json";
			qwr::QwrException::ExpectTrue(fs::exists(packageJsonFile), "Corrupted package: can't find `package.json`");

			parsedSettings.scriptPath = (packageDir / config::GetRelativePathToMainFile());
			parsedSettings.isSample = (packageDir.parent_path() == path::Packages_Sample());

			const auto str = TextFile(packageJsonFile.native()).read();
			const auto jsonMain = JSON::parse(str);
			qwr::QwrException::ExpectTrue(jsonMain.is_object(), "Corrupted `package.json`: not a JSON object");

			parsedSettings.packageId = jsonMain.at("id").get<std::string>();
			parsedSettings.scriptName = jsonMain.at("name").get<std::string>();
			parsedSettings.scriptAuthor = jsonMain.at("author").get<std::string>();
			parsedSettings.scriptVersion = jsonMain.at("version").get<std::string>();
			parsedSettings.scriptDescription = jsonMain.value("description", std::string());
			parsedSettings.enableDragDrop = jsonMain.value("enableDragDrop", false);
			parsedSettings.shouldGrabFocus = jsonMain.value("shouldGrabFocus", true);
		}
		catch (const fs::filesystem_error& e)
		{
			throw qwr::QwrException(e);
		}
		catch (const JSON::exception& e)
		{
			throw qwr::QwrException("Corrupted `package.json`: {}", e.what());
		}
	}

	void Save_PackageData(const config::ParsedPanelSettings& parsedSettings)
	{
		namespace fs = std::filesystem;

		assert(parsedSettings.scriptPath);
		qwr::QwrException::ExpectTrue(!parsedSettings.scriptPath->empty(), "Corrupted settings: `scriptPath` is empty");

		try
		{
			auto jsonMain = JSON::object();

			assert(parsedSettings.packageId);
			qwr::QwrException::ExpectTrue(!parsedSettings.packageId->empty(), "Corrupted settings: `id` is empty");

			jsonMain.push_back({ "id", *parsedSettings.packageId });
			jsonMain.push_back({ "name", parsedSettings.scriptName });
			jsonMain.push_back({ "author", parsedSettings.scriptAuthor });
			jsonMain.push_back({ "version", parsedSettings.scriptVersion });
			jsonMain.push_back({ "description", parsedSettings.scriptDescription });
			jsonMain.push_back({ "enableDragDrop", parsedSettings.enableDragDrop });
			jsonMain.push_back({ "shouldGrabFocus", parsedSettings.shouldGrabFocus });

			const auto packageDirRet = config::FindPackage(*parsedSettings.packageId);
			const auto packagePath = [&] {
				if (packageDirRet)
				{
					return *packageDirRet;
				}
				else
				{
					return GetPackagePath(parsedSettings);
				}
			}();

			if (!fs::exists(packagePath))
			{
				fs::create_directories(packagePath);
			}

			const auto packageJsonFile = packagePath / L"package.json";
			TextFile(packageJsonFile.native()).write(jsonMain.dump(2));

			const auto mainScriptPath = packagePath / config::GetRelativePathToMainFile();
			if (!fs::exists(mainScriptPath))
			{
				TextFile(mainScriptPath.native()).write(config::PanelSettings_InMemory::GetDefaultScript());
			}
		}
		catch (const fs::filesystem_error& e)
		{
			throw qwr::QwrException(e);
		}
		catch (const JSON::exception& e)
		{
			throw qwr::QwrException("Corrupted settings: {}", e.what());
		}
	}
}

namespace smp::config
{
	const fs::path& GetRelativePathToMainFile()
	{
		static const fs::path main{ "main.js" };
		return main;
	}

	ParsedPanelSettings GetNewPackageSettings(const std::string& name)
	{
		ParsedPanelSettings settings;

		try
		{
			fs::path packagePath;
			std::string id;
			do
			{
				const auto guidStr = utils::GuidToStr(utils::GenerateGuid());
				id = qwr::ToU8(guidStr);
				packagePath = path::Packages_Profile() / id;
			} while (fs::exists(packagePath));

			settings.packageId = id;
			settings.scriptName = name;
			settings.scriptPath = (packagePath / GetRelativePathToMainFile());
		}
		catch (const fs::filesystem_error& e)
		{
			throw qwr::QwrException(e);
		}

		return settings;
	}

	std::optional<std::filesystem::path> FindPackage(const std::string& packageId)
	{

		try
		{
			for (const auto& path: { path::Packages_Sample(), path::Packages_Profile(), path::Packages_Foobar2000() })
			{
				const auto targetPath = path / packageId;
				if (fs::exists(targetPath) && fs::is_directory(targetPath))
				{
					return targetPath;
				}
			}
		}
		catch (const fs::filesystem_error& e)
		{
			throw qwr::QwrException(e);
		}

		return std::nullopt;
	}

	ParsedPanelSettings GetPackageSettingsFromPath(const std::filesystem::path& packagePath)
	{
		ParsedPanelSettings settings{};
		Parse_PackageFromPath(packagePath, settings);
		return settings;
	}

	void FillPackageSettingsFromPath(const std::filesystem::path& packagePath, ParsedPanelSettings& settings)
	{
		Parse_PackageFromPath(packagePath, settings);
	}

	void MaybeSavePackageData(const ParsedPanelSettings& settings)
	{
		if (settings.GetSourceType() == ScriptSourceType::Package)
		{
			Save_PackageData(settings);
		}
	}

	std::filesystem::path GetPackageScriptsDir(const ParsedPanelSettings& settings)
	{
		return GetPackagePath(settings) / "scripts";
	}

	std::filesystem::path GetPackageAssetsDir(const ParsedPanelSettings& settings)
	{
		return GetPackagePath(settings) / "assets";
	}

	std::filesystem::path GetPackageStorageDir(const ParsedPanelSettings& settings)
	{
		return path::Packages_Storage() / *settings.packageId;
	}

	std::filesystem::path GetPackagePath(const ParsedPanelSettings& settings)
	{
		return settings.scriptPath->parent_path();
	}

	WStrings GetPackageFiles(const ParsedPanelSettings& settings)
	{
		auto files = GetPackageScriptFiles(settings);
		const auto packagePath = GetPackagePath(settings);
		const auto assetsDir = packagePath / "assets";
		auto assetFiles = DirectoryIterator(assetsDir).list_files(true);
		ranges::actions::push_back(files, std::move(assetFiles));
		return files;
	}

	WStrings GetPackageScriptFiles(const ParsedPanelSettings& settings)
	{
		const auto packagePath = GetPackagePath(settings);
		const auto scriptsDir = packagePath / "scripts";
		auto files = DirectoryIterator(scriptsDir).list_files(true);

		const auto mainScript = *settings.scriptPath;
		ranges::actions::remove_if(files, [&mainScript](const auto& path) { return (fs::path(path).extension() != ".js" || path == mainScript); });

		files.insert(files.begin(), mainScript);
		return files;
	}
}
