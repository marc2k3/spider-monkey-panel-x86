#pragma once

class QwrHookHandler
{
private:
	using HookCallback = std::function<void(int, WPARAM, LPARAM)>;

public:
	~QwrHookHandler();

	static QwrHookHandler& GetInstance();

	/// @throw smp::SmpException
	template <typename T>
	uint32_t RegisterHook(T&& callback)
	{
		if (s_callbacks.empty())
		{
			MaybeRegisterGlobalHook();
		}

		uint32_t id = m_cur_id++;

		while (s_callbacks.contains(id) || !id)
		{
			id = m_cur_id++;
		}

		s_callbacks.emplace(id, std::make_shared<HookCallback>(std::move(callback)));
		return id;
	}

	void UnregisterHook(uint32_t hookId);

private:
	QwrHookHandler() = default;

	static LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam);
	static inline std::unordered_map<uint32_t, std::shared_ptr<HookCallback>> s_callbacks;

	/// @throw smp::SmpException
	void MaybeRegisterGlobalHook();

	// TODO: add handlers for different hooks if needed
	HHOOK m_hook{};
	uint32_t m_cur_id = 1;
};
