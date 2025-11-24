#include <stdafx.h>
#include "hook_handler.h"

QwrHookHandler::~QwrHookHandler()
{
	if (m_hook)
	{
		::UnhookWindowsHookEx(m_hook);
	}
}

QwrHookHandler& QwrHookHandler::GetInstance()
{
	static QwrHookHandler hh;
	return hh;
}

void QwrHookHandler::UnregisterHook(uint32_t hookId)
{
	s_callbacks.erase(hookId);
}

void QwrHookHandler::MaybeRegisterGlobalHook()
{
	if (!m_hook)
	{
		m_hook = ::SetWindowsHookExW(WH_GETMESSAGE, GetMsgProc, nullptr, ::GetCurrentThreadId());
		qwr::CheckWinApi(m_hook, "SetWindowsHookEx");
	}
}

LRESULT CALLBACK QwrHookHandler::GetMsgProc(int code, WPARAM wParam, LPARAM lParam)
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
