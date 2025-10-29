#include <stdafx.h>
#include "component_defines.h"

namespace smp
{
	pfc::string8 about_text()
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
		).c_str();
	}

	void about_popup(HWND parent)
	{
		popup_message_v3::get()->messageBox(parent, about_text(), "About " SMP_NAME, MB_SETFOREGROUND);
	}

	DECLARE_COMPONENT_VERSION(SMP_NAME, SMP_VERSION, about_text());
	VALIDATE_COMPONENT_FILENAME(SMP_DLL_NAME);
}
