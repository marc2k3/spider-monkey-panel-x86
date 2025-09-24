#include <stdafx.h>

#include "window.h"

#include <config/package_utils.h>
#include <events/event_basic.h>
#include <events/event_dispatcher.h>
#include <events/event_notify_others.h>
#include <js_engine/js_engine.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_tooltip.h>
#include <js_objects/gdi_font.h>
#include <js_objects/internal/fb_properties.h>
#include <js_objects/menu_object.h>
#include <js_objects/theme_manager.h>
#include <js_utils/js_async_task.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_property_helper.h>
#include <panel/js_panel_window.h>
#include <timeout/timeout_manager.h>
#include <utils/gdi_helpers.h>

#include <libPPUI/win32_utility.h>
#include <qwr/winapi_error_helpers.h>

using namespace smp;

namespace
{

class TimeoutJsTask
	: public mozjs::JsAsyncTaskImpl<JS::HandleValue, JS::HandleValue>
{
public:
	TimeoutJsTask(JSContext* cx, JS::HandleValue funcValue, JS::HandleValue argArrayValue);
	~TimeoutJsTask() override = default;

private:
	/// @throw JsException
	bool InvokeJsImpl(JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue funcValue, JS::HandleValue argArrayValue) override;
};

TimeoutJsTask::TimeoutJsTask(JSContext* cx, JS::HandleValue funcValue, JS::HandleValue argArrayValue)
	: JsAsyncTaskImpl(cx, funcValue, argArrayValue)
{
}

bool TimeoutJsTask::InvokeJsImpl(JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue funcValue, JS::HandleValue argArrayValue)
{
	JS::RootedFunction jsFunc(cx, JS_ValueToFunction(cx, funcValue));
	JS::RootedObject jsArrayObject(cx, argArrayValue.toObjectOrNull());
	assert(jsArrayObject);

	bool is;
	if (!JS::IsArrayObject(cx, jsArrayObject, &is))
	{
		throw smp::JsException();
	}
	assert(is);

	uint32_t arraySize;
	if (!JS::GetArrayLength(cx, jsArrayObject, &arraySize))
	{
		throw smp::JsException();
	}

	JS::RootedValueVector jsVector(cx);
	if (arraySize)
	{
		if (!jsVector.reserve(arraySize))
		{
			throw std::bad_alloc();
		}

		JS::RootedValue arrayElement(cx);
		for (uint32_t i = 0; i < arraySize; ++i)
		{
			if (!JS_GetElement(cx, jsArrayObject, i, &arrayElement))
			{
				throw smp::JsException();
			}

			if (!jsVector.emplaceBack(arrayElement))
			{
				throw std::bad_alloc();
			}
		}
	}

	JS::RootedValue dummyRetVal(cx);
	return JS::Call(cx, jsGlobal, jsFunc, jsVector, &dummyRetVal);
}

} // namespace

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	Window::FinalizeJsObject,
	nullptr,
	nullptr,
	nullptr,
	Window::Trace
};

JSClass jsClass = {
	"Window",
	kDefaultClassFlags,
	&jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE(ClearInterval, Window::ClearInterval)
MJS_DEFINE_JS_FN_FROM_NATIVE(ClearTimeout, Window::ClearTimeout)
MJS_DEFINE_JS_FN_FROM_NATIVE(CreatePopupMenu, Window::CreatePopupMenu)
MJS_DEFINE_JS_FN_FROM_NATIVE(CreateThemeManager, Window::CreateThemeManager)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(CreateTooltip, Window::CreateTooltip, Window::CreateTooltipWithOpt, 3)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(DefinePanel, Window::DefinePanel, Window::DefinePanelWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(DefineScript, Window::DefineScript, Window::DefineScriptWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(EditScript, Window::EditScript)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(GetColourCUI, Window::GetColourCUI, Window::GetColourCUIWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(GetColourDUI, Window::GetColourDUI)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(GetFontCUI, Window::GetFontCUI, Window::GetFontCUIWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(GetFontDUI, Window::GetFontDUI)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(GetProperty, Window::GetProperty, Window::GetPropertyWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(NotifyOthers, Window::NotifyOthers)
MJS_DEFINE_JS_FN_FROM_NATIVE(Reload, Window::Reload)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(Repaint, Window::Repaint, Window::RepaintWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(RepaintRect, Window::RepaintRect, Window::RepaintRectWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(SetCursor, Window::SetCursor)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(SetInterval, Window::SetInterval, Window::SetIntervalWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(SetProperty, Window::SetProperty, Window::SetPropertyWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(SetTimeout, Window::SetTimeout, Window::SetTimeoutWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(ShowConfigure, Window::ShowConfigure)
MJS_DEFINE_JS_FN_FROM_NATIVE(ShowConfigureV2, Window::ShowConfigureV2)
MJS_DEFINE_JS_FN_FROM_NATIVE(ShowProperties, Window::ShowProperties)

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
	{
		JS_FN("ClearInterval", ClearInterval, 1, kDefaultPropsFlags),
		JS_FN("ClearTimeout", ClearTimeout, 1, kDefaultPropsFlags),
		JS_FN("CreatePopupMenu", CreatePopupMenu, 0, kDefaultPropsFlags),
		JS_FN("CreateThemeManager", CreateThemeManager, 1, kDefaultPropsFlags),
		JS_FN("CreateTooltip", CreateTooltip, 0, kDefaultPropsFlags),
		JS_FN("DefinePanel", DefinePanel, 1, kDefaultPropsFlags),
		JS_FN("DefineScript", DefineScript, 1, kDefaultPropsFlags),
		JS_FN("EditScript", EditScript, 0, kDefaultPropsFlags),
		JS_FN("GetColourCUI", GetColourCUI, 1, kDefaultPropsFlags),
		JS_FN("GetColourDUI", GetColourDUI, 1, kDefaultPropsFlags),
		JS_FN("GetFontCUI", GetFontCUI, 1, kDefaultPropsFlags),
		JS_FN("GetFontDUI", GetFontDUI, 1, kDefaultPropsFlags),
		JS_FN("GetProperty", GetProperty, 1, kDefaultPropsFlags),
		JS_FN("NotifyOthers", NotifyOthers, 2, kDefaultPropsFlags),
		JS_FN("Reload", Reload, 0, kDefaultPropsFlags),
		JS_FN("Repaint", Repaint, 0, kDefaultPropsFlags),
		JS_FN("RepaintRect", RepaintRect, 4, kDefaultPropsFlags),
		JS_FN("SetCursor", SetCursor, 1, kDefaultPropsFlags),
		JS_FN("SetInterval", SetInterval, 2, kDefaultPropsFlags),
		JS_FN("SetProperty", SetProperty, 1, kDefaultPropsFlags),
		JS_FN("SetTimeout", SetTimeout, 2, kDefaultPropsFlags),
		JS_FN("ShowConfigure", ShowConfigure, 0, kDefaultPropsFlags),
		JS_FN("ShowConfigureV2", ShowConfigureV2, 0, kDefaultPropsFlags),
		JS_FN("ShowProperties", ShowProperties, 0, kDefaultPropsFlags),
		JS_FS_END,
	});

MJS_DEFINE_JS_FN_FROM_NATIVE(get_DlgCode, Window::get_DlgCode)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_DPI, Window::get_DPI)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_Height, Window::get_Height)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_ID, Window::get_ID)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_InstanceType, Window::get_InstanceType)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_IsDark, Window::get_IsDark)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_IsTransparent, Window::get_IsTransparent)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_IsVisible, Window::get_IsVisible)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_JsMemoryStats, Window::get_JsMemoryStats)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_MaxHeight, Window::get_MaxHeight)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_MaxWidth, Window::get_MaxWidth)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_MemoryLimit, Window::get_MemoryLimit)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_MinHeight, Window::get_MinHeight)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_MinWidth, Window::get_MinWidth)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_Name, Window::get_Name)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_PanelMemoryUsage, Window::get_PanelMemoryUsage)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_ScriptInfo, Window::get_ScriptInfo)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_Tooltip, Window::get_Tooltip)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_TotalMemoryUsage, Window::get_TotalMemoryUsage)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_Width, Window::get_Width)
MJS_DEFINE_JS_FN_FROM_NATIVE(put_DlgCode, Window::put_DlgCode)
MJS_DEFINE_JS_FN_FROM_NATIVE(put_MaxHeight, Window::put_MaxHeight)
MJS_DEFINE_JS_FN_FROM_NATIVE(put_MaxWidth, Window::put_MaxWidth)
MJS_DEFINE_JS_FN_FROM_NATIVE(put_MinHeight, Window::put_MinHeight)
MJS_DEFINE_JS_FN_FROM_NATIVE(put_MinWidth, Window::put_MinWidth)

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
	{
		JS_PSGS("DlgCode", get_DlgCode, put_DlgCode, kDefaultPropsFlags),
		JS_PSG("DPI", get_DPI, kDefaultPropsFlags),
		JS_PSG("Height", get_Height, kDefaultPropsFlags),
		JS_PSG("ID", get_ID, kDefaultPropsFlags),
		JS_PSG("InstanceType", get_InstanceType, kDefaultPropsFlags),
		JS_PSG("IsDark", get_IsDark, kDefaultPropsFlags),
		JS_PSG("IsTransparent", get_IsTransparent, kDefaultPropsFlags),
		JS_PSG("IsVisible", get_IsVisible, kDefaultPropsFlags),
		JS_PSG("JsMemoryStats", get_JsMemoryStats, kDefaultPropsFlags),
		JS_PSGS("MaxHeight", get_MaxHeight, put_MaxHeight, kDefaultPropsFlags),
		JS_PSGS("MaxWidth", get_MaxWidth, put_MaxWidth, kDefaultPropsFlags),
		JS_PSG("MemoryLimit", get_MemoryLimit, kDefaultPropsFlags),
		JS_PSGS("MinHeight", get_MinHeight, put_MinHeight, kDefaultPropsFlags),
		JS_PSGS("MinWidth", get_MinWidth, put_MinWidth, kDefaultPropsFlags),
		JS_PSG("Name", get_Name, kDefaultPropsFlags),
		JS_PSG("PanelMemoryUsage", get_PanelMemoryUsage, kDefaultPropsFlags),
		JS_PSG("ScriptInfo", get_ScriptInfo, kDefaultPropsFlags),
		JS_PSG("Tooltip", get_Tooltip, kDefaultPropsFlags),
		JS_PSG("TotalMemoryUsage", get_TotalMemoryUsage, kDefaultPropsFlags),
		JS_PSG("Width", get_Width, kDefaultPropsFlags),
		JS_PS_END,
	});

} // namespace

namespace mozjs
{

const JSClass Window::JsClass = jsClass;
const JSFunctionSpec* Window::JsFunctions = jsFunctions.data();
const JSPropertySpec* Window::JsProperties = jsProperties.data();

Window::~Window()
{
}

Window::Window(JSContext* cx, smp::panel::js_panel_window& parentPanel, std::unique_ptr<FbProperties> fbProperties)
	: pJsCtx_(cx)
	, parentPanel_(parentPanel)
	, fbProperties_(std::move(fbProperties))
{
}

std::unique_ptr<Window>
Window::CreateNative(JSContext* cx, smp::panel::js_panel_window& parentPanel)
{
	std::unique_ptr<FbProperties> fbProperties = FbProperties::Create(cx, parentPanel);
	if (!fbProperties)
	{ // report in Create
		return nullptr;
	}

	return std::unique_ptr<Window>(new Window(cx, parentPanel, std::move(fbProperties)));
}

size_t Window::GetInternalSize(const smp::panel::js_panel_window&)
{
	return sizeof(FbProperties);
}

void Window::Trace(JSTracer* trc, JSObject* obj)
{
	auto x = static_cast<Window*>(JS::GetPrivate(obj));
	if (x && x->fbProperties_)
	{
		x->fbProperties_->Trace(trc);
	}
}

void Window::PrepareForGc()
{
	if (fbProperties_)
	{
		fbProperties_->PrepareForGc();
		fbProperties_.reset();
	}
	if (pNativeTooltip_)
	{
		assert(pNativeTooltip_);
		pNativeTooltip_->PrepareForGc();
		pNativeTooltip_ = nullptr;
		jsTooltip_.reset();
	}

	isFinalized_ = true;
}

HWND Window::GetHwnd() const
{
	if (isFinalized_)
	{
		return nullptr;
	}

	return parentPanel_.GetHWND();
}

void Window::ClearInterval(uint32_t intervalId) const
{
	if (isFinalized_)
	{
		return;
	}

	parentPanel_.GetTimeoutManager().ClearTimeout(intervalId);
}

void Window::ClearTimeout(uint32_t timeoutId) const
{
	if (isFinalized_)
	{
		return;
	}

	parentPanel_.GetTimeoutManager().ClearTimeout(timeoutId);
}

JSObject* Window::CreatePopupMenu()
{
	if (isFinalized_)
	{
		return nullptr;
	}

	return JsMenuObject::CreateJs(pJsCtx_, parentPanel_.GetHWND());
}

JSObject* Window::CreateThemeManager(const std::wstring& classid)
{
	if (isFinalized_)
	{
		return nullptr;
	}

	if (!JsThemeManager::HasThemeData(parentPanel_.GetHWND(), classid))
	{ // Not a error: not found
		return nullptr;
	}

	return JsThemeManager::CreateJs(pJsCtx_, parentPanel_.GetHWND(), classid);
}

JSObject* Window::CreateTooltip(const std::wstring& name, uint32_t pxSize, uint32_t style)
{
	if (isFinalized_)
	{
		return nullptr;
	}

	if (!jsTooltip_.initialized())
	{
		jsTooltip_.init(pJsCtx_, JsFbTooltip::CreateJs(pJsCtx_, parentPanel_.GetHWND()));
		pNativeTooltip_ = static_cast<JsFbTooltip*>(JS::GetPrivate(jsTooltip_));
	}

	assert(pNativeTooltip_);
	pNativeTooltip_->SetFont(name, pxSize, style);

	return jsTooltip_;
}

JSObject* Window::CreateTooltipWithOpt(size_t optArgCount, const std::wstring& name, uint32_t pxSize, uint32_t style)
{
	switch (optArgCount)
	{
	case 0:
		return CreateTooltip(name, pxSize, style);
	case 1:
		return CreateTooltip(name, pxSize);
	case 2:
		return CreateTooltip(name);
	case 3:
		return CreateTooltip();
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void Window::DefinePanel(const std::string& name, JS::HandleValue options)
{
	qwr::QwrException::ExpectTrue(
		parentPanel_.GetSettings().GetSourceType() != config::ScriptSourceType::Package,
		"`DefinePanel` can't be used to change package script information - use `Configure` instead");
	qwr::QwrException::ExpectTrue(!isScriptDefined_, "DefinePanel/DefineScript can't be called twice");

	const auto parsedOptions = ParseDefineScriptOptions(options);

	parentPanel_.SetSettings_PanelName(name);
	parentPanel_.SetSettings_ScriptInfo(name, parsedOptions.author, parsedOptions.version);
	parentPanel_.SetSettings_DragAndDropStatus(parsedOptions.features.dragAndDrop);
	parentPanel_.SetSettings_CaptureFocusStatus(parsedOptions.features.grabFocus);

	isScriptDefined_ = true;
}

void Window::DefinePanelWithOpt(size_t optArgCount, const std::string& name, JS::HandleValue options)
{
	switch (optArgCount)
	{
	case 0:
		return DefinePanel(name, options);
	case 1:
		return DefinePanel(name);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void Window::DefineScript(const std::string& name, JS::HandleValue options)
{
	if (isFinalized_)
	{
		return;
	}

	qwr::QwrException::ExpectTrue(
		parentPanel_.GetSettings().GetSourceType() != config::ScriptSourceType::Package,
		"`DefineScript` can't be used to change package script information - use `Configure` instead");
	qwr::QwrException::ExpectTrue(!isScriptDefined_, "DefineScript can't be called twice");

	const auto parsedOptions = ParseDefineScriptOptions(options);

	parentPanel_.SetSettings_ScriptInfo(name, parsedOptions.author, parsedOptions.version);
	parentPanel_.SetSettings_DragAndDropStatus(parsedOptions.features.dragAndDrop);
	parentPanel_.SetSettings_CaptureFocusStatus(parsedOptions.features.grabFocus);

	isScriptDefined_ = true;
}

void Window::DefineScriptWithOpt(size_t optArgCount, const std::string& name, JS::HandleValue options)
{
	switch (optArgCount)
	{
	case 0:
		return DefineScript(name, options);
	case 1:
		return DefineScript(name);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void Window::EditScript()
{
	if (isFinalized_)
	{
		return;
	}

	EventDispatcher::Get().PutEvent(parentPanel_.GetHWND(), std::make_unique<Event_Basic>(EventId::kScriptEdit), EventPriority::kControl);
}

uint32_t Window::GetColourCUI(uint32_t type, const std::wstring& guidstr)
{
	if (isFinalized_)
	{
		return 0;
	}

	qwr::QwrException::ExpectTrue(parentPanel_.GetPanelType() == panel::PanelType::CUI, "Can be called only in CUI");
	qwr::QwrException::ExpectTrue(type <= cui::colours::colour_active_item_frame, "Invalid colour type specified");

	GUID guid{};

	if (!guidstr.empty())
	{
		HRESULT hr = CLSIDFromString(guidstr.c_str(), &guid);
		qwr::error::CheckHR(hr, "CLSIDFromString");
	}

	return parentPanel_.GetColour(guid, type);
}

uint32_t Window::GetColourCUIWithOpt(size_t optArgCount, uint32_t type, const std::wstring& guidstr)
{
	switch (optArgCount)
	{
	case 0:
		return GetColourCUI(type, guidstr);
	case 1:
		return GetColourCUI(type);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

uint32_t Window::GetColourDUI(uint32_t type)
{
	static constexpr std::array guids =
	{
		&ui_color_text,
		&ui_color_background,
		&ui_color_highlight,
		&ui_color_selection,
	};

	if (isFinalized_)
	{
		return 0;
	}

	qwr::QwrException::ExpectTrue(parentPanel_.GetPanelType() == panel::PanelType::DUI, "Can be called only in DUI");
	qwr::QwrException::ExpectTrue(type < guids.size(), "Invalid colour type specified");

	return parentPanel_.GetColour(*guids[type]);
}

JSObject* Window::GetFontCUI(uint32_t type, const std::wstring& guidstr)
{
	if (isFinalized_)
	{
		return nullptr;
	}

	qwr::QwrException::ExpectTrue(parentPanel_.GetPanelType() == panel::PanelType::CUI, "Can be called only in CUI");
	qwr::QwrException::ExpectTrue(type <= cui::fonts::font_type_labels, "Invalid font type specified");

	GUID guid{};

	if (!guidstr.empty())
	{
		HRESULT hr = CLSIDFromString(guidstr.c_str(), &guid);
		qwr::error::CheckHR(hr, "CLSIDFromString");
	}

	auto hFont = gdi::CreateUniquePtr(parentPanel_.GetFont(guid, type));
	std::unique_ptr<Gdiplus::Font> pGdiFont(new Gdiplus::Font(parentPanel_.GetHDC(), hFont.get()));

	if (gdi::IsGdiPlusObjectValid(pGdiFont))
	{
		return JsGdiFont::CreateJs(pJsCtx_, std::move(pGdiFont), hFont.release(), true);
	}

	return nullptr;
}

JSObject* Window::GetFontCUIWithOpt(size_t optArgCount, uint32_t type, const std::wstring& guidstr)
{
	switch (optArgCount)
	{
	case 0:
		return GetFontCUI(type, guidstr);
	case 1:
		return GetFontCUI(type);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

JSObject* Window::GetFontDUI(uint32_t type)
{
	static constexpr std::array guids =
	{
		&ui_font_default,
		&ui_font_tabs,
		&ui_font_lists,
		&ui_font_playlists,
		&ui_font_statusbar,
		&ui_font_console,
	};

	if (isFinalized_)
	{
		return nullptr;
	}

	qwr::QwrException::ExpectTrue(parentPanel_.GetPanelType() == panel::PanelType::DUI, "Can be called only in DUI");
	qwr::QwrException::ExpectTrue(type < guids.size(), "Invalid font type specified");

	auto hFont = parentPanel_.GetFont(*guids[type]); 
	std::unique_ptr<Gdiplus::Font> pGdiFont(new Gdiplus::Font(parentPanel_.GetHDC(), hFont));

	if (gdi::IsGdiPlusObjectValid(pGdiFont))
	{
		return JsGdiFont::CreateJs(pJsCtx_, std::move(pGdiFont), hFont, false);
	}

	return nullptr;
}

JS::Value Window::GetProperty(const std::wstring& name, JS::HandleValue defaultval)
{
	if (isFinalized_)
	{
		return JS::UndefinedValue();
	}

	return fbProperties_->GetProperty(name, defaultval);
}

JS::Value Window::GetPropertyWithOpt(size_t optArgCount, const std::wstring& name, JS::HandleValue defaultval)
{
	switch (optArgCount)
	{
	case 0:
		return GetProperty(name, defaultval);
	case 1:
		return GetProperty(name);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void Window::NotifyOthers(const std::wstring& name, JS::HandleValue info)
{
	if (isFinalized_)
	{
		return;
	}

	EventDispatcher::Get().NotifyOthers(parentPanel_.GetHWND(), std::make_unique<Event_NotifyOthers>(pJsCtx_, name, info));
}

void Window::Reload()
{
	if (isFinalized_)
	{
		return;
	}

	EventDispatcher::Get().PutEvent(parentPanel_.GetHWND(), std::make_unique<Event_Basic>(EventId::kScriptReload), EventPriority::kControl);
}

void Window::Repaint(bool force)
{
	if (isFinalized_)
	{
		return;
	}

	parentPanel_.Repaint(force);
}

void Window::RepaintWithOpt(size_t optArgCount, bool force)
{
	switch (optArgCount)
	{
	case 0:
		return Repaint(force);
	case 1:
		return Repaint();
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void Window::RepaintRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force)
{
	if (isFinalized_)
	{
		return;
	}

	parentPanel_.RepaintRect(CRect{ static_cast<int>(x), static_cast<int>(y), static_cast<int>(x + w), static_cast<int>(y + h) }, force);
}

void Window::RepaintRectWithOpt(size_t optArgCount, uint32_t x, uint32_t y, uint32_t w, uint32_t h, bool force)
{
	switch (optArgCount)
	{
	case 0:
		return RepaintRect(x, y, w, h, force);
	case 1:
		return RepaintRect(x, y, w, h);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void Window::SetCursor(uint32_t id)
{
	if (isFinalized_)
	{
		return;
	}

	::SetCursor(LoadCursor(nullptr, MAKEINTRESOURCE(id)));
}

uint32_t Window::SetInterval(JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs)
{
	if (isFinalized_)
	{
		return 0;
	}

	qwr::QwrException::ExpectTrue(func.isObject() && JS_ObjectIsFunction(&func.toObject()), "`func` argument is not a JS function");
	qwr::QwrException::ExpectTrue(delay > 0, "`delay` must be non-zero");

	JS::RootedFunction jsFunction(pJsCtx_, JS_ValueToFunction(pJsCtx_, func));
	JS::RootedValue jsFuncValue(pJsCtx_, JS::ObjectValue(*JS_GetFunctionObject(jsFunction)));

	JS::RootedObject jsArrayObject(pJsCtx_, JS::NewArrayObject(pJsCtx_, funcArgs));
	smp::JsException::ExpectTrue(jsArrayObject);
	JS::RootedValue jsArrayValue(pJsCtx_, JS::ObjectValue(*jsArrayObject));

	return parentPanel_.GetTimeoutManager().SetInterval(delay, std::make_unique<TimeoutJsTask>(pJsCtx_, jsFuncValue, jsArrayValue));
}

uint32_t Window::SetIntervalWithOpt(size_t optArgCount, JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs)
{
	switch (optArgCount)
	{
	case 0:
		return SetInterval(func, delay, funcArgs);
	case 1:
		return SetInterval(func, delay);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void Window::SetProperty(const std::wstring& name, JS::HandleValue val)
{
	if (isFinalized_)
	{
		return;
	}

	fbProperties_->SetProperty(name, val);
}

void Window::SetPropertyWithOpt(size_t optArgCount, const std::wstring& name, JS::HandleValue val)
{
	switch (optArgCount)
	{
	case 0:
		return SetProperty(name, val);
	case 1:
		return SetProperty(name);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

uint32_t Window::SetTimeout(JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs)
{
	if (isFinalized_)
	{
		return 0;
	}

	qwr::QwrException::ExpectTrue(func.isObject() && JS_ObjectIsFunction(&func.toObject()), "func argument is not a JS function");

	JS::RootedFunction jsFunction(pJsCtx_, JS_ValueToFunction(pJsCtx_, func));
	JS::RootedValue jsFuncValue(pJsCtx_, JS::ObjectValue(*JS_GetFunctionObject(jsFunction)));

	JS::RootedObject jsArrayObject(pJsCtx_, JS::NewArrayObject(pJsCtx_, funcArgs));
	smp::JsException::ExpectTrue(jsArrayObject);
	JS::RootedValue jsArrayValue(pJsCtx_, JS::ObjectValue(*jsArrayObject));

	return parentPanel_.GetTimeoutManager().SetTimeout(delay, std::make_unique<TimeoutJsTask>(pJsCtx_, jsFuncValue, jsArrayValue));
}

uint32_t Window::SetTimeoutWithOpt(size_t optArgCount, JS::HandleValue func, uint32_t delay, JS::HandleValueArray funcArgs)
{
	switch (optArgCount)
	{
	case 0:
		return SetTimeout(func, delay, funcArgs);
	case 1:
		return SetTimeout(func, delay);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void Window::ShowConfigure()
{
	if (isFinalized_)
	{
		return;
	}

	EventDispatcher::Get().PutEvent(parentPanel_.GetHWND(), std::make_unique<Event_Basic>(EventId::kScriptShowConfigureLegacy), EventPriority::kControl);
}

void Window::ShowConfigureV2()
{
	if (isFinalized_)
	{
		return;
	}

	EventDispatcher::Get().PutEvent(parentPanel_.GetHWND(), std::make_unique<Event_Basic>(EventId::kScriptShowConfigure), EventPriority::kControl);
}

void Window::ShowProperties()
{
	if (isFinalized_)
	{
		return;
	}

	EventDispatcher::Get().PutEvent(parentPanel_.GetHWND(), std::make_unique<Event_Basic>(EventId::kScriptShowProperties), EventPriority::kControl);
}

uint32_t Window::get_DlgCode()
{
	if (isFinalized_)
	{
		return 0;
	}

	return parentPanel_.DlgCode();
}

uint32_t Window::get_DPI()
{
	static const auto dpi = QueryScreenDPI(core_api::get_main_window());
	return dpi;
}

uint32_t Window::get_Height()
{
	if (isFinalized_)
	{
		return 0;
	}

	return parentPanel_.GetHeight();
}

uint32_t Window::get_ID() const
{
	// Such cast works properly only on x86
	return reinterpret_cast<uint32_t>(GetHwnd());
}

uint32_t Window::get_InstanceType()
{
	if (isFinalized_)
	{
		return 0;
	}

	return static_cast<uint32_t>(parentPanel_.GetPanelType());
}

bool Window::get_IsDark()
{
	return parentPanel_.IsDark();
}

bool Window::get_IsTransparent()
{
	if (isFinalized_)
	{
		return false;
	}

	return parentPanel_.GetSettings().isPseudoTransparent;
}

bool Window::get_IsVisible()
{
	if (isFinalized_)
	{
		return false;
	}

	return IsWindowVisible(parentPanel_.GetHWND());
}

JSObject* Window::get_JsMemoryStats()
{
	if (isFinalized_)
	{
		return nullptr;
	}

	JS::RootedObject jsObject(pJsCtx_, JS_NewPlainObject(pJsCtx_));

	JS::RootedObject jsGlobal(pJsCtx_, JS::CurrentGlobalOrNull(pJsCtx_));
	AddProperty(pJsCtx_, jsObject, "MemoryUsage", JsGc::GetTotalHeapUsageForGlobal(pJsCtx_, jsGlobal));
	AddProperty(pJsCtx_, jsObject, "TotalMemoryUsage", JsEngine::GetInstance().GetGcEngine().GetTotalHeapUsage());
	AddProperty(pJsCtx_, jsObject, "TotalMemoryLimit", JsGc::GetMaxHeap());

	return jsObject;
}

uint32_t Window::get_MaxHeight()
{
	if (isFinalized_)
	{
		return 0;
	}

	return parentPanel_.MaxSize().y;
}

uint32_t Window::get_MaxWidth()
{
	if (isFinalized_)
	{
		return 0;
	}

	return parentPanel_.MaxSize().x;
}

uint32_t Window::get_MemoryLimit() const
{
	if (isFinalized_)
	{
		return 0;
	}

	return JsGc::GetMaxHeap();
}

uint32_t Window::get_MinHeight()
{
	if (isFinalized_)
	{
		return 0;
	}

	return parentPanel_.MinSize().y;
}

uint32_t Window::get_MinWidth()
{
	if (isFinalized_)
	{
		return 0;
	}

	return parentPanel_.MinSize().x;
}

std::string Window::get_Name()
{
	if (isFinalized_)
	{
		return std::string{};
	}

	return parentPanel_.GetPanelId();
}

uint64_t Window::get_PanelMemoryUsage()
{
	if (isFinalized_)
	{
		return 0;
	}

	JS::RootedObject jsGlobal(pJsCtx_, JS::CurrentGlobalOrNull(pJsCtx_));
	return JsGc::GetTotalHeapUsageForGlobal(pJsCtx_, jsGlobal);
}

JSObject* Window::get_ScriptInfo()
{
	if (isFinalized_)
	{
		return nullptr;
	}

	const auto& settings = parentPanel_.GetSettings();

	JS::RootedObject jsObject(pJsCtx_, JS_NewPlainObject(pJsCtx_));

	AddProperty(pJsCtx_, jsObject, "Name", settings.scriptName);
	if (!settings.scriptAuthor.empty())
	{
		AddProperty(pJsCtx_, jsObject, "Author", settings.scriptAuthor);
	}
	if (!settings.scriptVersion.empty())
	{
		AddProperty(pJsCtx_, jsObject, "Version", settings.scriptVersion);
	}
	if (settings.packageId)
	{
		AddProperty(pJsCtx_, jsObject, "PackageId", *settings.packageId);
	}

	return jsObject;
}

uint64_t Window::get_TotalMemoryUsage() const
{
	if (isFinalized_)
	{
		return 0;
	}

	return JsEngine::GetInstance().GetGcEngine().GetTotalHeapUsage();
}

JSObject* Window::get_Tooltip()
{
	return CreateTooltip();
}

uint32_t Window::get_Width()
{
	if (isFinalized_)
	{
		return 0;
	}

	return parentPanel_.GetWidth();
}

void Window::put_DlgCode(uint32_t code)
{
	if (isFinalized_)
	{
		return;
	}

	parentPanel_.DlgCode() = code;
}

void Window::put_MaxHeight(uint32_t height)
{
	if (isFinalized_)
	{
		return;
	}

	parentPanel_.MaxSize().y = height;
	parentPanel_.NotifySizeLimitChanged();
}

void Window::put_MaxWidth(uint32_t width)
{
	if (isFinalized_)
	{
		return;
	}

	parentPanel_.MaxSize().x = width;
	parentPanel_.NotifySizeLimitChanged();
}

void Window::put_MinHeight(uint32_t height)
{
	if (isFinalized_)
	{
		return;
	}

	parentPanel_.MinSize().y = height;
	parentPanel_.NotifySizeLimitChanged();
}

void Window::put_MinWidth(uint32_t width)
{
	if (isFinalized_)
	{
		return;
	}

	parentPanel_.MinSize().x = width;
	parentPanel_.NotifySizeLimitChanged();
}

Window::DefineScriptOptions Window::ParseDefineScriptOptions(JS::HandleValue options)
{
	DefineScriptOptions parsedOptions;
	if (!options.isNullOrUndefined())
	{
		qwr::QwrException::ExpectTrue(options.isObject(), "options argument is not an object");
		JS::RootedObject jsOptions(pJsCtx_, &options.toObject());

		parsedOptions.author = GetOptionalProperty<std::string>(pJsCtx_, jsOptions, "author").value_or("");
		parsedOptions.version = GetOptionalProperty<std::string>(pJsCtx_, jsOptions, "version").value_or("");

		bool hasProperty;
		if (!JS_HasProperty(pJsCtx_, jsOptions, "features", &hasProperty))
		{
			throw JsException();
		}

		if (hasProperty)
		{
			JS::RootedValue jsFeaturesValue(pJsCtx_);
			if (!JS_GetProperty(pJsCtx_, jsOptions, "features", &jsFeaturesValue))
			{
				throw JsException();
			}

			qwr::QwrException::ExpectTrue(jsFeaturesValue.isObject(), "`features` is not an object");

			JS::RootedObject jsFeatures(pJsCtx_, &jsFeaturesValue.toObject());
			parsedOptions.features.dragAndDrop = GetOptionalProperty<bool>(pJsCtx_, jsFeatures, "drag_n_drop").value_or(false);
			parsedOptions.features.grabFocus = GetOptionalProperty<bool>(pJsCtx_, jsFeatures, "grab_focus").value_or(true);
		}
	}

	return parsedOptions;
}

} // namespace mozjs
