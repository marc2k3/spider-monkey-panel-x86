#include <stdafx.h>

#include "js_engine.h"

#include <com_utils/com_destruction_handler.h>
#include <fb2k/advanced_config.h>
#include <js_engine/heartbeat_window.h>
#include <js_engine/js_container.h>
#include <js_engine/js_internal_global.h>
#include <js_engine/js_realm_inner.h>
#include <js_objects/global_object.h>
#include <js_utils/js_error_helper.h>
#include <panel/js_panel_window.h>
#include <panel/modal_blocking_scope.h>
#include <panel/user_message.h>
#include <timeout/timer_interface.h>

#include <js/Initialization.h>
#include <qwr/error_popup.h>
#include <qwr/string_helpers.h>

using namespace smp;

namespace
{
	constexpr uint32_t kHeartbeatRateMs = 73;
	[[maybe_unused]] constexpr uint32_t kJobsMaxBudgetMs = 500;

	// Half the size of the actual C stack, to be safe (default stack size in VS is 1MB).
	constexpr size_t kMaxStackLimit = 1024LL * 1024 / 2;

	template <typename T, typename D>
	[[nodiscard]] std::unique_ptr<T, D> make_unique_with_dtor(T* t, D d)
	{
		return std::unique_ptr<T, D>(t, d);
	}

	void ReportException(const std::string& errorText) noexcept
	{
		const std::string errorTextPadded = [&errorText]()
			{
				std::string text = "Critical JS engine error: " SMP_NAME_WITH_VERSION;
				if (!errorText.empty())
				{
					text += "\n";
					text += errorText;
				}

				return text;
			}();

		qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, errorTextPadded);
	}
}

namespace mozjs
{

JsEngine::JsEngine()
{
	JS_Init();
}

JsEngine::~JsEngine() {}

JsEngine& JsEngine::GetInstance() noexcept
{
	static JsEngine je;
	return je;
}

void JsEngine::PrepareForExit() noexcept
{
	shouldShutdown_ = true;
	if (registeredContainers_.empty())
	{ // finalize immediately, since we don't have containers to care about
		Finalize();
	}
}

bool JsEngine::RegisterContainer(JsContainer& jsContainer) noexcept
{
	if (registeredContainers_.empty() && !Initialize())
	{
		return false;
	}

	jsContainer.SetJsCtx(pJsCtx_);
	registeredContainers_.emplace(&jsContainer, jsContainer);
	jsMonitor_.AddContainer(jsContainer);

	return true;
}

void JsEngine::UnregisterContainer(JsContainer& jsContainer) noexcept
{
	if (auto it = registeredContainers_.find(&jsContainer); it != registeredContainers_.end())
	{
		jsMonitor_.RemoveContainer(jsContainer);

		it->second.get().Finalize();
		registeredContainers_.erase(it);
	}

	if (registeredContainers_.empty())
	{
		Finalize();
	}
}

void JsEngine::MaybeRunJobs() noexcept
{
	if (!isInitialized_)
	{
		return;
	}

	// TODO: add ability for user to abort script here

	if (areJobsInProgress_)
	{ // might occur when called from nested loop
		/*
		// Use this only for script abort with error
		if (timeGetTime() - jobsStartTime_ >= kJobsMaxBudgetMs)
		{
			js::StopDrainingJobQueue(pJsCtx_);
		}
		*/
		return;
	}

	jobsStartTime_ = timeGetTime();
	areJobsInProgress_ = true;
	auto autoJobs = wil::scope_exit([&] {
		areJobsInProgress_ = false;
	});

	{
		js::RunJobs(pJsCtx_);

		for (size_t i = 0; i < rejectedPromises_.length(); ++i)
		{
			const auto& rejectedPromise = rejectedPromises_[i];
			if (!rejectedPromise)
			{
				continue;
			}

			JSAutoRealm ac(pJsCtx_, rejectedPromise);
			mozjs::error::AutoJsReport are(pJsCtx_);

			JS::RootedValue jsValue(pJsCtx_, JS::GetPromiseResult(rejectedPromise));
			if (!jsValue.isNullOrUndefined())
			{
				JS_SetPendingException(pJsCtx_, jsValue);
			}
			else
			{ // Should not reach here, mostly paranoia check
				JS_ReportErrorUTF8(pJsCtx_, "Unhandled promise rejection");
			}
		}
		rejectedPromises_.get().clear();
	}
}

void JsEngine::OnJsActionStart(JsContainer& jsContainer) noexcept
{
	jsMonitor_.OnJsActionStart(jsContainer);
}

void JsEngine::OnJsActionEnd(JsContainer& jsContainer) noexcept
{
	jsMonitor_.OnJsActionEnd(jsContainer);
}

JsGc& JsEngine::GetGcEngine() noexcept
{
	return jsGc_;
}

const JsGc& JsEngine::GetGcEngine() const noexcept
{
	return jsGc_;
}

JsInternalGlobal& JsEngine::GetInternalGlobal() noexcept
{
	return *internalGlobal_;
}

void JsEngine::OnHeartbeat() noexcept
{
	if (!isInitialized_ || isBeating_ || shouldStopHeartbeatThread_)
	{
		return;
	}

	isBeating_ = true;

	{
		if (!jsGc_.MaybeGc())
		{ // OOM
			ReportOomError();
		}
	}

	isBeating_ = false;
}

bool JsEngine::Initialize() noexcept
{
	if (isInitialized_)
	{
		return true;
	}

	auto autoJsCtx = make_unique_with_dtor<JSContext>(nullptr, [](auto pCtx)
		{
			JS_DestroyContext(pCtx);
		});

	try
	{
		autoJsCtx.reset(JS_NewContext(JsGc::GetMaxHeap()));
		qwr::QwrException::ExpectTrue(autoJsCtx.get(), "JS_NewContext failed");

		JSContext* cx = autoJsCtx.get();

		JS_SetNativeStackQuota(cx, kMaxStackLimit);

		if (!JS_AddInterruptCallback(cx, InterruptHandler))
		{
			throw JsException();
		}

		if (!js::UseInternalJobQueues(cx))
		{
			throw JsException();
		}

		JS::SetPromiseRejectionTrackerCallback(cx, RejectedPromiseHandler, this);

		// TODO: JS::SetWarningReporter(pJsCtx_)

		if (!JS::InitSelfHostedCode(cx))
		{
			throw JsException();
		}

		jsGc_.Initialize(cx);

		rejectedPromises_.init(cx, JS::GCVector<JSObject*, 0, js::SystemAllocPolicy>(js::SystemAllocPolicy()));
		internalGlobal_ = JsInternalGlobal::Create(cx);

		StartHeartbeatThread();
		jsMonitor_.Start(cx);
	}
	catch (const JsException&)
	{
		ReportException(mozjs::error::JsErrorToText(autoJsCtx.get()));
		return false;
	}
	catch (const qwr::QwrException& e)
	{
		ReportException(e.what());
		return false;
	}

	pJsCtx_ = autoJsCtx.release();
	isInitialized_ = true;

	return true;
}

void JsEngine::Finalize() noexcept
{
	if (pJsCtx_)
	{
		jsMonitor_.Stop();
		// Stop the thread first, so that we don't get additional GC's during jsGc.Finalize
		StopHeartbeatThread();
		jsGc_.Finalize();

		internalGlobal_.reset();
		rejectedPromises_.reset();

		JS_DestroyContext(pJsCtx_);
		pJsCtx_ = nullptr;
	}

	if (shouldShutdown_)
	{
		TimerManager_Custom::Get().Finalize();
		TimerManager_Native::Get().Finalize();
		JS_ShutDown();
		smp::com::DeleteAllStoredObject();
	}

	isInitialized_ = false;
}

void JsEngine::StartHeartbeatThread() noexcept
{
	if (!heartbeatWindow_)
	{
		heartbeatWindow_ = smp::HeartbeatWindow::Create();
	}

	shouldStopHeartbeatThread_ = false;
	heartbeatThread_ = std::thread([parent = this]
		{
			while (!parent->shouldStopHeartbeatThread_)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(kHeartbeatRateMs));
				PostMessageW(parent->heartbeatWindow_->GetHwnd(), static_cast<UINT>(smp::MiscMessage::heartbeat), 0, 0);
			}
		});
}

void JsEngine::StopHeartbeatThread() noexcept
{
	if (heartbeatThread_.joinable())
	{
		shouldStopHeartbeatThread_ = true;
		heartbeatThread_.join();
	}
}

bool JsEngine::InterruptHandler(JSContext*) noexcept
{
	return JsEngine::GetInstance().OnInterrupt();
}

bool JsEngine::OnInterrupt() noexcept
{
	return jsMonitor_.OnInterrupt();
}

void JsEngine::RejectedPromiseHandler(JSContext*, bool mutedErrors, JS::HandleObject promise, JS::PromiseRejectionHandlingState state, void* data) noexcept
{
	JsEngine& self = *reinterpret_cast<JsEngine*>(data);

	if (JS::PromiseRejectionHandlingState::Handled == state)
	{
		auto& uncaughtRejections = self.rejectedPromises_;
		for (size_t i = 0; i < uncaughtRejections.length(); ++i)
		{
			if (uncaughtRejections[i] == promise)
			{
				// To avoid large amounts of memmoves, we don't shrink the vector here.
				// Instead, we filter out nullptrs when iterating over the vector later.
				uncaughtRejections[i].set(nullptr);
				break;
			}
		}
	}
	else
	{
		self.rejectedPromises_.get().append(promise);
	}
}

void JsEngine::ReportOomError() noexcept
{
	for (auto& [hWnd, jsContainer]: registeredContainers_)
	{
		auto& jsContainerRef = jsContainer.get();
		if (mozjs::JsContainer::JsStatus::Working != jsContainerRef.GetStatus())
		{
			continue;
		}

		jsContainerRef.Fail(fmt::format("Out of memory: {}/{} bytes", jsContainerRef.pNativeRealm_->GetCurrentHeapBytes(), JsGc::GetMaxHeap()));
	}
}

} // namespace mozjs
