#include <stdafx.h>
#include "logging.h"

namespace smp::utils
{
	void LogError(const std::string& message)
	{
		FB2K_console_formatter() << fmt::format(SMP_UNDERSCORE_NAME ":\nError:\n{}\n",	message);
	}

	void LogWarning(const std::string& message)
	{
		FB2K_console_formatter() << fmt::format(SMP_UNDERSCORE_NAME ":\nWarning:\n{}\n", message);
	}

	void LogDebug(const std::string& message)
	{
		FB2K_console_formatter() << fmt::format(SMP_UNDERSCORE_NAME ":\nDebug:\n{}\n", message);
	}
}
