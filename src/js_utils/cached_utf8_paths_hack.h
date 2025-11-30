#pragma once

// This is needed to support `GetCurrentScriptPath` hack and as a workaround for
// https://github.com/TheQwertiest/foo_spider_monkey_panel/issues/1

namespace mozjs::hack
{
	using Map = std::unordered_map<std::string, std::string>;

	/// @brief This is a hack, don't use it unless it's REALLY necessary
	/// @throw QwrException
	/// @throw smp::JsException
	[[nodiscard]] std::string CacheUtf8Path(std::string_view path);

	/// @brief This is a hack, don't use it unless it's REALLY necessary
	[[nodiscard]] std::optional<std::filesystem::path> GetCachedUtf8Path(std::string_view pathId);

	/// @brief This is a hack, don't use it unless it's REALLY necessary
	[[nodiscard]] const Map& GetAllCachedUtf8Paths();

	/// @brief This is a hack, don't use it unless it's REALLY necessary
	/// @throw QwrException
	/// @throw smp::JsException
	[[nodiscard]] std::optional<std::filesystem::path> GetCurrentScriptPath(JSContext* cx);
}
