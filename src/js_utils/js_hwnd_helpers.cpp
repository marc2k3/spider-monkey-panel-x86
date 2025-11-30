#include <stdafx.h>
#include "js_hwnd_helpers.h"

namespace mozjs
{
	HWND GetPanelHwndForCurrentGlobal(JSContext* cx)
	{
		JS::RootedObject jsGlobal(cx, JS::CurrentGlobalOrNull(cx));
		const auto pNativeGlobal = static_cast<JsGlobalObject*>(JS_GetInstancePrivate(cx, jsGlobal, &JsGlobalObject::JsClass, nullptr));
		return pNativeGlobal->GetPanelHwnd();
	}
}
