#include <stdafx.h>
#include "cached_utf8_paths_hack.h"

namespace mozjs::hack
{
	Map g_cachedPaths;

	std::string CacheUtf8Path(std::string_view path)
	{
		const std::string hash = hasher_md5::get()->process_single_string(path.data()).asString().get_ptr();
		g_cachedPaths.try_emplace(hash, path);
		return hash;
	}

	std::optional<std::filesystem::path> GetCachedUtf8Path(std::string_view pathId)
	{
		const auto it = g_cachedPaths.find({ pathId.data(), pathId.size() });
		if (it == g_cachedPaths.end())
		{
			return std::nullopt;
		}

		return qwr::ToWide(it->second);
	}

	const Map& GetAllCachedUtf8Paths()
	{
		return g_cachedPaths;
	}

	std::optional<std::filesystem::path> GetCurrentScriptPath(JSContext* cx)
	{
		try
		{
			JS_ReportErrorUTF8(cx, "hacking around...");
			assert(JS_IsExceptionPending(cx));

			JS::ExceptionStack excn(cx);
			(void)JS::StealPendingExceptionStack(cx, &excn);

			JS::ErrorReportBuilder reportBuilder(cx);
			if (!reportBuilder.init(cx, excn, JS::ErrorReportBuilder::SniffingBehavior::WithSideEffects))
			{
				throw QwrException("ErrorReportBuilder::init failed");
			}

			JSErrorReport* pReport = reportBuilder.report();
			assert(pReport);

			if (!pReport->filename || std::string(pReport->filename).empty())
			{
				return std::nullopt;
			}

			// workaround for https://github.com/TheQwertiest/foo_spider_monkey_panel/issues/1
			// and https://bugzilla.mozilla.org/show_bug.cgi?id=1492090
			const auto pathOpt = hack::GetCachedUtf8Path(pReport->filename);
			if (!pathOpt)
			{
				return std::nullopt;
			}

			return pathOpt;
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			throw QwrException(e);
		}
	}
}
