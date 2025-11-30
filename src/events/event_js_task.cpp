#include <stdafx.h>
#include "event_js_task.h"

namespace smp
{
	Event_JsTask::Event_JsTask(EventId id, std::shared_ptr<mozjs::JsAsyncTask> pTask) : Event_JsExecutor(id), pTask_(pTask) {}

	std::optional<bool> Event_JsTask::JsExecute(mozjs::JsContainer& jsContainer)
	{
		jsContainer.InvokeJsAsyncTask(*pTask_);
		return std::nullopt;
	}
}
