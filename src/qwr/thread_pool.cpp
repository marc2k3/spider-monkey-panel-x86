#include <stdafx.h>

#include "thread_pool.h"

namespace qwr
{
	ThreadPool::ThreadPool(size_t maxThreadCount) : maxThreadCount_(std::max<size_t>(std::min(std::thread::hardware_concurrency(), maxThreadCount), 1))
	{
		threads_.reserve(maxThreadCount_);
	}

	void ThreadPool::Finalize() noexcept
	{
		{
			std::unique_lock sl(queueMutex_);
			isExiting_ = true;
		}

		hasTask_.notify_all();

		for (const auto& thread: threads_)
		{
			if (thread->joinable())
			{
				thread->join();
			}
		}

		threads_.clear();

		while (!tasks_.empty())
		{ // Might be non-empty if thread was aborted
			tasks_.pop();
		}
	}

	void ThreadPool::AddThread() noexcept
	{
		threads_.emplace_back(std::make_unique<std::thread>([&] { ThreadProc(); }));
	}

	void ThreadPool::ThreadProc() noexcept
	{
		++idleThreadCount_;
		auto scope = wil::scope_exit([&idleThreadsCount = idleThreadCount_]
			{
				--idleThreadsCount;
			});

		while (true)
		{
			if (isExiting_)
			{
				return;
			}

			std::unique_ptr<Task> task;
			{
				std::unique_lock sl(queueMutex_);
				hasTask_.wait(sl, [&tasks = tasks_, &isExiting = isExiting_]
					{
						return (!tasks.empty() || isExiting);
					});

				if (isExiting_)
					return;

				task.swap(tasks_.front());
				tasks_.pop();
			}

			--idleThreadCount_;

			try
			{
				std::invoke(*task);
			}
			catch (const std::exception& e)
			{
				FB2K_console_formatter() << "QWR Thread Pool (error):\n" << e.what();
			}

			++idleThreadCount_;
		}
	}
}
