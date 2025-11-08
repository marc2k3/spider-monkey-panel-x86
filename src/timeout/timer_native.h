#pragma once
#include <timeout/time_types.h>
#include <timeout/timer_interface_fwd.h>

namespace smp
{
	class PanelTarget;
	class TimerManager_Native;

	class Timer_Native final : public ITimer, public std::enable_shared_from_this<Timer_Native>
	{
	public:
		~Timer_Native() = default;

		void Start(TimerNotifyTask& task, const TimeStamp& when);
		void Cancel(bool waitForDestruction);

		void Fire(uint64_t generation);

		[[nodiscard]] PanelTarget& Target() const;
		[[nodiscard]] const TimeStamp& When() const;
		[[nodiscard]] uint64_t Generation() const;

	private:
		Timer_Native(TimerManager_Native& pParent, std::shared_ptr<PanelTarget> pTarget);

		static VOID CALLBACK TimerProc(PVOID lpParameter, BOOLEAN TimerOrWaitFired);

	private:
		friend class TimerManager_Native;

		TimerManager_Native& pParent_;
		std::shared_ptr<PanelTarget> pTarget_;
		HANDLE hTimer_ = nullptr;

		TimerNotifyTask* pTask_ = nullptr;
		TimeStamp executeAt_{};
		int64_t generation_ = 0;
	};
}
