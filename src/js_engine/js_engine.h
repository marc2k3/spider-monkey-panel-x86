#pragma once
#include <js_engine/js_gc.h>
#include <js_engine/js_monitor.h>

namespace smp
{

class HeartbeatWindow;
}

namespace mozjs
{

class JsContainer;
class JsInternalGlobal;

class JsEngine final
{
public:
	~JsEngine();
	JsEngine(const JsEngine&) = delete;
	JsEngine& operator=(const JsEngine&) = delete;

	static [[nodiscard]] JsEngine& GetInstance() noexcept;
	void PrepareForExit() noexcept;

public: // methods accessed by JsContainer
	[[nodiscard]] bool RegisterContainer(JsContainer& jsContainer) noexcept;
	void UnregisterContainer(JsContainer& jsContainer) noexcept;

	void MaybeRunJobs() noexcept;

	void OnJsActionStart(JsContainer& jsContainer) noexcept;
	void OnJsActionEnd(JsContainer& jsContainer) noexcept;

public: // methods accessed by js objects
	[[nodiscard]] JsGc& GetGcEngine() noexcept;
	[[nodiscard]] const JsGc& GetGcEngine() const noexcept;
	[[nodiscard]] JsInternalGlobal& GetInternalGlobal() noexcept;

public: // methods accessed by other internals
	void OnHeartbeat() noexcept;
	[[nodiscard]] bool OnInterrupt() noexcept;

private:
	JsEngine();

private:
	bool Initialize() noexcept;
	void Finalize() noexcept;

	/// @throw qwr::QwrException
	void StartHeartbeatThread() noexcept;
	void StopHeartbeatThread() noexcept;

	static bool InterruptHandler(JSContext* cx) noexcept;

	static void RejectedPromiseHandler(
		JSContext* cx,
		bool mutedErrors,
		JS::HandleObject promise,
		JS::PromiseRejectionHandlingState state,
		void* data) noexcept;

	void ReportOomError() noexcept;

private:
	JSContext* pJsCtx_ = nullptr;

	bool isInitialized_ = false;
	bool shouldShutdown_ = false;

	std::map<void*, std::reference_wrapper<JsContainer>> registeredContainers_;

	bool isBeating_ = false;
	std::unique_ptr<smp::HeartbeatWindow> heartbeatWindow_;
	std::thread heartbeatThread_;
	std::atomic_bool shouldStopHeartbeatThread_ = false;

	JsGc jsGc_;
	JsMonitor jsMonitor_;

	JS::PersistentRooted<JS::GCVector<JSObject*, 0, js::SystemAllocPolicy>> rejectedPromises_;
	bool areJobsInProgress_ = false;
	uint32_t jobsStartTime_ = 0;

	std::unique_ptr<JsInternalGlobal> internalGlobal_;
};

} // namespace mozjs
