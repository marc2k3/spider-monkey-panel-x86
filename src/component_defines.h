#pragma once

#define SMP_NAME              "Spider Monkey Panel"
#define SMP_UNDERSCORE_NAME   "foo_spider_monkey_panel"
#define SMP_WINDOW_CLASS_NAME SMP_UNDERSCORE_NAME "_class"
#define SMP_DLL_NAME          SMP_UNDERSCORE_NAME ".dll"

#define SMP_VERSION "1.6.2.25.10.16"
#define SMP_NAME_WITH_VERSION SMP_NAME " v" SMP_VERSION

static std::string about_smp()
{
	const auto date = pfc::string(__DATE__).replace("  ", " ");
	const auto msvc = fmt::to_string(_MSC_FULL_VER);

	return fmt::format(
		SMP_NAME_WITH_VERSION " by TheQwertiest\n"
		"Based on JScript Panel by marc2003\n"
		"Based on WSH Panel Mod by T.P. Wang\n\n"
		"Build: {}, {}\n\n"
		"foobar2000 SDK: {}\n"
		"Columns UI SDK: {}\n"
		"MSVC: {}.{}.{}",
		__TIME__,
		date.get_ptr(),
		FOOBAR2000_SDK_VERSION,
		UI_EXTENSION_VERSION,
		msvc.substr(0, 2),
		msvc.substr(2, 2),
		msvc.substr(4)
	);
}
