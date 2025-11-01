#include <stdafx.h>
#include "hook_handler.h"
#include <qwr/winapi_error_helpers.h>

namespace qwr
{
	HookHandler::~HookHandler()
	{
		if (m_hook)
		{
			::UnhookWindowsHookEx(m_hook);
		}
	}

	HookHandler& HookHandler::GetInstance()
	{
		static HookHandler hh;
		return hh;
	}

	void HookHandler::UnregisterHook(uint32_t hookId)
	{
		s_callbacks.erase(hookId);
	}

	void HookHandler::MaybeRegisterGlobalHook()
	{
		if (!m_hook)
		{
			m_hook = ::SetWindowsHookExW(WH_GETMESSAGE, GetMsgProc, nullptr, ::GetCurrentThreadId());
			qwr::error::CheckWinApi(m_hook, "SetWindowsHookEx");
		}
	}

	LRESULT CALLBACK HookHandler::GetMsgProc(int code, WPARAM wParam, LPARAM lParam)
	{
		for (auto it = s_callbacks.begin(); it != s_callbacks.end();)
		{
			// callback might trigger self-destruction, thus we need to preserve it
			auto tmpCallback = *it->second;
			++it;
			std::invoke(tmpCallback, code, wParam, lParam);
		}

		return CallNextHookEx(nullptr, code, wParam, lParam);
	}
}
