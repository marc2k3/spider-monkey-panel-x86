#pragma once

#define SMP_NAME              "Spider Monkey Panel"
#define SMP_UNDERSCORE_NAME   "foo_spider_monkey_panel"
#define SMP_WINDOW_CLASS_NAME SMP_UNDERSCORE_NAME "_class"
#define SMP_DLL_NAME          SMP_UNDERSCORE_NAME ".dll"

#define SMP_VERSION "1.6.2.25.11.15"
#define SMP_NAME_WITH_VERSION SMP_NAME " v" SMP_VERSION
#define SMP_USER_AGENT SMP_DLL_NAME "/" SMP_VERSION

namespace smp
{
	void about_popup(HWND parent);
}
