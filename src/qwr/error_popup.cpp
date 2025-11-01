#include <stdafx.h>
#include "error_popup.h"

namespace qwr
{
	void ReportErrorWithPopup(const std::string& title, const std::string& errorText)
	{
		fb2k::inMainThread([title, errorText]
			{
				FB2K_console_formatter() << title << ":\n" << errorText;
				popup_message::g_show(errorText.c_str(), title.c_str());
				MessageBeep(MB_ICONASTERISK);
			});
	}

	void ReportFSErrorWithPopup(const std::filesystem::filesystem_error& e)
	{
		const auto errorText = qwr::FS_Error_ToU8(e);
		ReportErrorWithPopup(SMP_UNDERSCORE_NAME, errorText);
	}
}
