#include <stdafx.h>

#include "fb.h"

#include <2K3/AlbumArtStatic.hpp>
#include <com_objects/drop_source_impl.h>
#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>
#include <fb2k/mainmenu_dynamic.h>
#include <fb2k/selection_holder_helper.h>
#include <fb2k/stats.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/context_menu_manager.h>
#include <js_objects/fb_audio_chunk.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list.h>
#include <js_objects/fb_profiler.h>
#include <js_objects/fb_title_format.h>
#include <js_objects/fb_ui_selection_holder.h>
#include <js_objects/gdi_bitmap.h>
#include <js_objects/main_menu_manager.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_hwnd_helpers.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_property_helper.h>
#include <panel/modal_blocking_scope.h>
#include <panel/user_message.h>
#include <utils/menu_helpers.h>

#include <qwr/string_helpers.h>

using namespace smp;

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
	Fb::FinalizeJsObject,
	nullptr,
	nullptr,
	nullptr,
	nullptr
};

JSClass jsClass = {
	"FbUtils",
	kDefaultClassFlags,
	&jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE(AcquireUiSelectionHolder, Fb::AcquireUiSelectionHolder)
MJS_DEFINE_JS_FN_FROM_NATIVE(AddDirectory, Fb::AddDirectory)
MJS_DEFINE_JS_FN_FROM_NATIVE(AddFiles, Fb::AddFiles)
MJS_DEFINE_JS_FN_FROM_NATIVE(AddLocationsAsync, Fb::AddLocationsAsync);
MJS_DEFINE_JS_FN_FROM_NATIVE(CheckClipboardContents, Fb::CheckClipboardContents)
MJS_DEFINE_JS_FN_FROM_NATIVE(ClearPlaylist, Fb::ClearPlaylist)
MJS_DEFINE_JS_FN_FROM_NATIVE(CopyHandleListToClipboard, Fb::CopyHandleListToClipboard)
MJS_DEFINE_JS_FN_FROM_NATIVE(CreateContextMenuManager, Fb::CreateContextMenuManager)
MJS_DEFINE_JS_FN_FROM_NATIVE(CreateHandleList, Fb::CreateHandleList)
MJS_DEFINE_JS_FN_FROM_NATIVE(CreateMainMenuManager, Fb::CreateMainMenuManager)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(CreateProfiler, Fb::CreateProfiler, Fb::CreateProfilerWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(DoDragDrop, Fb::DoDragDrop, Fb::DoDragDropWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(Exit, Fb::Exit)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(GetAudioChunk, Fb::GetAudioChunk, Fb::GetAudioChunkWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(GetClipboardContents, Fb::GetClipboardContents, Fb::GetClipboardContentsWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(GetDSPPresets, Fb::GetDSPPresets)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(GetFocusItem, Fb::GetFocusItem, Fb::GetFocusItemWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(GetLibraryItems, Fb::GetLibraryItems)
MJS_DEFINE_JS_FN_FROM_NATIVE(GetLibraryRelativePath, Fb::GetLibraryRelativePath)
MJS_DEFINE_JS_FN_FROM_NATIVE(GetNowPlaying, Fb::GetNowPlaying)
MJS_DEFINE_JS_FN_FROM_NATIVE(GetOutputDevices, Fb::GetOutputDevices)
MJS_DEFINE_JS_FN_FROM_NATIVE(GetQueryItems, Fb::GetQueryItems)
MJS_DEFINE_JS_FN_FROM_NATIVE(GetSelection, Fb::GetSelection)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(GetSelections, Fb::GetSelections, Fb::GetSelectionsWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(GetSelectionType, Fb::GetSelectionType)
MJS_DEFINE_JS_FN_FROM_NATIVE(IsLibraryEnabled, Fb::IsLibraryEnabled)
MJS_DEFINE_JS_FN_FROM_NATIVE(IsMainMenuCommandChecked, Fb::IsMainMenuCommandChecked)
MJS_DEFINE_JS_FN_FROM_NATIVE(IsMetadbInMediaLibrary, Fb::IsMetadbInMediaLibrary)
MJS_DEFINE_JS_FN_FROM_NATIVE(LoadPlaylist, Fb::LoadPlaylist)
MJS_DEFINE_JS_FN_FROM_NATIVE(Next, Fb::Next)
MJS_DEFINE_JS_FN_FROM_NATIVE(Pause, Fb::Pause)
MJS_DEFINE_JS_FN_FROM_NATIVE(Play, Fb::Play)
MJS_DEFINE_JS_FN_FROM_NATIVE(PlayOrPause, Fb::PlayOrPause)
MJS_DEFINE_JS_FN_FROM_NATIVE(Prev, Fb::Prev)
MJS_DEFINE_JS_FN_FROM_NATIVE(Random, Fb::Random)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(RegisterMainMenuCommand, Fb::RegisterMainMenuCommand, Fb::RegisterMainMenuCommandWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(RequestBacklog, Fb::RequestBacklog)
MJS_DEFINE_JS_FN_FROM_NATIVE(Restart, Fb::Restart)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(RunContextCommand, Fb::RunContextCommand, Fb::RunContextCommandWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(RunContextCommandWithMetadb, Fb::RunContextCommandWithMetadb, Fb::RunContextCommandWithMetadbWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(RunMainMenuCommand, Fb::RunMainMenuCommand)
MJS_DEFINE_JS_FN_FROM_NATIVE(SavePlaylist, Fb::SavePlaylist)
MJS_DEFINE_JS_FN_FROM_NATIVE(SetDSPPreset, Fb::SetDSPPreset)
MJS_DEFINE_JS_FN_FROM_NATIVE(SetOutputDevice, Fb::SetOutputDevice)
MJS_DEFINE_JS_FN_FROM_NATIVE(ShowConsole, Fb::ShowConsole)
MJS_DEFINE_JS_FN_FROM_NATIVE(ShowLibrarySearchUI, Fb::ShowLibrarySearchUI)
MJS_DEFINE_JS_FN_FROM_NATIVE(ShowPictureViewer, Fb::ShowPictureViewer)
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(ShowPopupMessage, Fb::ShowPopupMessage, Fb::ShowPopupMessageWithOpt, 1)
MJS_DEFINE_JS_FN_FROM_NATIVE(ShowPreferences, Fb::ShowPreferences)
MJS_DEFINE_JS_FN_FROM_NATIVE(Stop, Fb::Stop)
MJS_DEFINE_JS_FN_FROM_NATIVE(TitleFormat, Fb::TitleFormat)
MJS_DEFINE_JS_FN_FROM_NATIVE(UnregisterMainMenuCommand, Fb::UnregisterMainMenuCommand)
MJS_DEFINE_JS_FN_FROM_NATIVE(VolumeDown, Fb::VolumeDown)
MJS_DEFINE_JS_FN_FROM_NATIVE(VolumeMute, Fb::VolumeMute)
MJS_DEFINE_JS_FN_FROM_NATIVE(VolumeUp, Fb::VolumeUp)

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
	{
		JS_FN("AcquireUiSelectionHolder", AcquireUiSelectionHolder, 0, kDefaultPropsFlags),
		JS_FN("AddDirectory", AddDirectory, 0, kDefaultPropsFlags),
		JS_FN("AddFiles", AddFiles, 0, kDefaultPropsFlags),
		JS_FN("AddLocationsAsync", AddLocationsAsync, 1, kDefaultPropsFlags),
		JS_FN("CheckClipboardContents", CheckClipboardContents, 0, kDefaultPropsFlags),
		JS_FN("ClearPlaylist", ClearPlaylist, 0, kDefaultPropsFlags),
		JS_FN("CopyHandleListToClipboard", CopyHandleListToClipboard, 1, kDefaultPropsFlags),
		JS_FN("CreateContextMenuManager", CreateContextMenuManager, 0, kDefaultPropsFlags),
		JS_FN("CreateHandleList", CreateHandleList, 0, kDefaultPropsFlags),
		JS_FN("CreateMainMenuManager", CreateMainMenuManager, 0, kDefaultPropsFlags),
		JS_FN("CreateProfiler", CreateProfiler, 0, kDefaultPropsFlags),
		JS_FN("DoDragDrop", DoDragDrop, 3, kDefaultPropsFlags),
		JS_FN("Exit", Exit, 0, kDefaultPropsFlags),
		JS_FN("GetAudioChunk", GetAudioChunk, 2, kDefaultPropsFlags),
		JS_FN("GetClipboardContents", GetClipboardContents, 0, kDefaultPropsFlags),
		JS_FN("GetDSPPresets", GetDSPPresets, 0, kDefaultPropsFlags),
		JS_FN("GetFocusItem", GetFocusItem, 0, kDefaultPropsFlags),
		JS_FN("GetLibraryItems", GetLibraryItems, 0, kDefaultPropsFlags),
		JS_FN("GetLibraryRelativePath", GetLibraryRelativePath, 1, kDefaultPropsFlags),
		JS_FN("GetNowPlaying", GetNowPlaying, 0, kDefaultPropsFlags),
		JS_FN("GetOutputDevices", GetOutputDevices, 0, kDefaultPropsFlags),
		JS_FN("GetQueryItems", GetQueryItems, 2, kDefaultPropsFlags),
		JS_FN("GetSelection", GetSelection, 0, kDefaultPropsFlags),
		JS_FN("GetSelections", GetSelections, 0, kDefaultPropsFlags),
		JS_FN("GetSelectionType", GetSelectionType, 0, kDefaultPropsFlags),
		JS_FN("IsLibraryEnabled", IsLibraryEnabled, 0, kDefaultPropsFlags),
		JS_FN("IsMainMenuCommandChecked", IsMainMenuCommandChecked, 1, kDefaultPropsFlags),
		JS_FN("IsMetadbInMediaLibrary", IsMetadbInMediaLibrary, 1, kDefaultPropsFlags),
		JS_FN("LoadPlaylist", LoadPlaylist, 0, kDefaultPropsFlags),
		JS_FN("Next", Next, 0, kDefaultPropsFlags),
		JS_FN("Pause", Pause, 0, kDefaultPropsFlags),
		JS_FN("Play", Play, 0, kDefaultPropsFlags),
		JS_FN("PlayOrPause", PlayOrPause, 0, kDefaultPropsFlags),
		JS_FN("Prev", Prev, 0, kDefaultPropsFlags),
		JS_FN("Random", Random, 0, kDefaultPropsFlags),
		JS_FN("RegisterMainMenuCommand", RegisterMainMenuCommand, 2, kDefaultPropsFlags),
		JS_FN("RequestBacklog", RequestBacklog, 1, kDefaultPropsFlags),
		JS_FN("Restart", Restart, 0, kDefaultPropsFlags),
		JS_FN("RunContextCommand", RunContextCommand, 1, kDefaultPropsFlags),
		JS_FN("RunContextCommandWithMetadb", RunContextCommandWithMetadb, 2, kDefaultPropsFlags),
		JS_FN("RunMainMenuCommand", RunMainMenuCommand, 1, kDefaultPropsFlags),
		JS_FN("SavePlaylist", SavePlaylist, 0, kDefaultPropsFlags),
		JS_FN("SetDSPPreset", SetDSPPreset, 1, kDefaultPropsFlags),
		JS_FN("SetOutputDevice", SetOutputDevice, 2, kDefaultPropsFlags),
		JS_FN("ShowConsole", ShowConsole, 0, kDefaultPropsFlags),
		JS_FN("ShowLibrarySearchUI", ShowLibrarySearchUI, 1, kDefaultPropsFlags),
		JS_FN("ShowPictureViewer", ShowPictureViewer, 1, kDefaultPropsFlags),
		JS_FN("ShowPopupMessage", ShowPopupMessage, 1, kDefaultPropsFlags),
		JS_FN("ShowPreferences", ShowPreferences, 0, kDefaultPropsFlags),
		JS_FN("Stop", Stop, 0, kDefaultPropsFlags),
		JS_FN("TitleFormat", TitleFormat, 1, kDefaultPropsFlags),
		JS_FN("UnregisterMainMenuCommand", UnregisterMainMenuCommand, 1, kDefaultPropsFlags),
		JS_FN("VolumeDown", VolumeDown, 0, kDefaultPropsFlags),
		JS_FN("VolumeMute", VolumeMute, 0, kDefaultPropsFlags),
		JS_FN("VolumeUp", VolumeUp, 0, kDefaultPropsFlags),
		JS_FS_END,
	});

MJS_DEFINE_JS_FN_FROM_NATIVE(get_AlwaysOnTop, Fb::get_AlwaysOnTop)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_ComponentPath, Fb::get_ComponentPath)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_CursorFollowPlayback, Fb::get_CursorFollowPlayback)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_CustomVolume, Fb::get_CustomVolume)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_FoobarPath, Fb::get_FoobarPath)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_IsPaused, Fb::get_IsPaused)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_IsPlaying, Fb::get_IsPlaying)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_PlaybackFollowCursor, Fb::get_PlaybackFollowCursor)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_PlaybackLength, Fb::get_PlaybackLength)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_PlaybackTime, Fb::get_PlaybackTime)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_ProfilePath, Fb::get_ProfilePath)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_ReplaygainMode, Fb::get_ReplaygainMode)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_StopAfterCurrent, Fb::get_StopAfterCurrent)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_Version, Fb::get_Version)
MJS_DEFINE_JS_FN_FROM_NATIVE(get_Volume, Fb::get_Volume)
MJS_DEFINE_JS_FN_FROM_NATIVE(put_AlwaysOnTop, Fb::put_AlwaysOnTop)
MJS_DEFINE_JS_FN_FROM_NATIVE(put_CursorFollowPlayback, Fb::put_CursorFollowPlayback)
MJS_DEFINE_JS_FN_FROM_NATIVE(put_PlaybackFollowCursor, Fb::put_PlaybackFollowCursor)
MJS_DEFINE_JS_FN_FROM_NATIVE(put_PlaybackTime, Fb::put_PlaybackTime)
MJS_DEFINE_JS_FN_FROM_NATIVE(put_ReplaygainMode, Fb::put_ReplaygainMode)
MJS_DEFINE_JS_FN_FROM_NATIVE(put_StopAfterCurrent, Fb::put_StopAfterCurrent)
MJS_DEFINE_JS_FN_FROM_NATIVE(put_Volume, Fb::put_Volume)

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
	{
		JS_PSGS("AlwaysOnTop", get_AlwaysOnTop, put_AlwaysOnTop, kDefaultPropsFlags),
		JS_PSG("ComponentPath", get_ComponentPath, kDefaultPropsFlags),
		JS_PSGS("CursorFollowPlayback", get_CursorFollowPlayback, put_CursorFollowPlayback, kDefaultPropsFlags),
		JS_PSG("CustomVolume", get_CustomVolume, kDefaultPropsFlags),
		JS_PSG("FoobarPath", get_FoobarPath, kDefaultPropsFlags),
		JS_PSG("IsPaused", get_IsPaused, kDefaultPropsFlags),
		JS_PSG("IsPlaying", get_IsPlaying, kDefaultPropsFlags),
		JS_PSGS("PlaybackFollowCursor", get_PlaybackFollowCursor, put_PlaybackFollowCursor, kDefaultPropsFlags),
		JS_PSG("PlaybackLength", get_PlaybackLength, kDefaultPropsFlags),
		JS_PSGS("PlaybackTime", get_PlaybackTime, put_PlaybackTime, kDefaultPropsFlags),
		JS_PSG("ProfilePath", get_ProfilePath, kDefaultPropsFlags),
		JS_PSGS("ReplaygainMode", get_ReplaygainMode, put_ReplaygainMode, kDefaultPropsFlags),
		JS_PSGS("StopAfterCurrent", get_StopAfterCurrent, put_StopAfterCurrent, kDefaultPropsFlags),
		JS_PSG("Version", get_Version, kDefaultPropsFlags),
		JS_PSGS("Volume", get_Volume, put_Volume, kDefaultPropsFlags),
		JS_PS_END,
	});

} // namespace

namespace mozjs
{

const JSClass Fb::JsClass = jsClass;
const JSFunctionSpec* Fb::JsFunctions = jsFunctions.data();
const JSPropertySpec* Fb::JsProperties = jsProperties.data();

Fb::Fb(JSContext* cx)
	: pJsCtx_(cx)
{
	visualisation_manager::get()->create_stream(vis_, visualisation_manager::KStreamFlagNewFFT);
}

std::unique_ptr<Fb>
Fb::CreateNative(JSContext* cx)
{
	return std::unique_ptr<Fb>(new Fb(cx));
}

size_t Fb::GetInternalSize()
{
	return 0;
}

JSObject* Fb::AcquireUiSelectionHolder()
{
	return JsFbUiSelectionHolder::CreateJs(pJsCtx_, ui_selection_manager::get()->acquire());
}

void Fb::AddDirectory()
{
	standard_commands::main_add_directory();
}

void Fb::AddFiles()
{
	standard_commands::main_add_files();
}

uint32_t Fb::AddLocationsAsync(JS::HandleValue locations)
{
	static uint32_t s_task_id{};
	static constexpr uint32_t s_flags = playlist_incoming_item_filter_v2::op_flag_no_filter | playlist_incoming_item_filter_v2::op_flag_delay_ui;

	const auto wnd = GetPanelHwndForCurrentGlobal(pJsCtx_);
	qwr::QwrException::ExpectTrue(wnd, "Method called before fb2k was initialized completely");

	pfc::string_list_impl location_list;
	convert::to_native::ProcessArray<std::string>(
		pJsCtx_,
		locations,
		[&location_list](const auto& location)
		{
			location_list.add_item(location.c_str());
		}
	);

	if (location_list.get_count() == 0)
		return 0;

	auto func = [wnd, task_id = ++s_task_id](metadb_handle_list_cref handles)
	{
		EventDispatcher::Get().PutEvent(
			wnd,
			GenerateEvent_JsCallback(
				EventId::kInternalLocationsAdded,
				task_id,
				std::make_shared<metadb_handle_list>(handles)
			)
		);
	};

	auto ptr = process_locations_notify::create(func);
	playlist_incoming_item_filter_v2::get()->process_locations_async(location_list, s_flags, nullptr, nullptr, nullptr, ptr);
	return s_task_id;
}

bool Fb::CheckClipboardContents()
{
	pfc::com_ptr_t<IDataObject> pDO;
	HRESULT hr = OleGetClipboard(pDO.receive_ptr());
	if (FAILED(hr))
	{
		return false;
	}

	bool native;
	DWORD drop_effect = DROPEFFECT_COPY;
	hr = ole_interaction::get()->check_dataobject(pDO, drop_effect, native);
	return SUCCEEDED(hr);
}

void Fb::ClearPlaylist()
{
	standard_commands::main_clear_playlist();
}

bool Fb::CopyHandleListToClipboard(JsFbMetadbHandleList* handles)
{
	qwr::QwrException::ExpectTrue(handles, "handles argument is null");

	pfc::com_ptr_t<IDataObject> pDO = ole_interaction::get()->create_dataobject(handles->GetHandleList());
	return SUCCEEDED(OleSetClipboard(pDO.get_ptr()));
}

JSObject* Fb::CreateContextMenuManager()
{
	return JsContextMenuManager::CreateJs(pJsCtx_);
}

JSObject* Fb::CreateHandleList()
{
	return JsFbMetadbHandleList::Constructor(pJsCtx_, JS::UndefinedHandleValue);
}

JSObject* Fb::CreateMainMenuManager()
{
	return JsMainMenuManager::CreateJs(pJsCtx_);
}

JSObject* Fb::CreateProfiler(const std::string& name)
{
	return JsFbProfiler::Constructor(pJsCtx_, name);
}

JSObject* Fb::CreateProfilerWithOpt(size_t optArgCount, const std::string& name)
{
	switch (optArgCount)
	{
	case 0:
		return CreateProfiler(name);
	case 1:
		return CreateProfiler();
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

uint32_t Fb::DoDragDrop(uint32_t, JsFbMetadbHandleList* handles, uint32_t okEffects, JS::HandleValue options)
{
	const auto wnd = GetPanelHwndForCurrentGlobal(pJsCtx_);
	qwr::QwrException::ExpectTrue(wnd, "Method called before fb2k was initialized completely");
	qwr::QwrException::ExpectTrue(handles, "handles argument is null");

	const metadb_handle_list& handleList = handles->GetHandleList();
	const size_t handleCount = handleList.get_count();

	std::unique_ptr<Gdiplus::Bitmap> autoImage;
	auto parsedOptions = ParseDoDragDropOptions(options);
	if (!parsedOptions.pCustomImage && parsedOptions.useAlbumArt && handleCount)
	{
		metadb_handle_ptr metadb;
		(void)playlist_manager::get()->activeplaylist_get_focus_item_handle(metadb);
		if (!metadb.is_valid() || pfc_infinite == handleList.find_item(metadb))
		{
			metadb = handleList[handleCount - 1];
		}
	}

	if (modal::IsModalBlocked() || !handleCount || okEffects == DROPEFFECT_NONE)
	{
		return DROPEFFECT_NONE;
	}

	modal::MessageBlockingScope scope;

	pfc::com_ptr_t<IDataObject> pDO = ole_interaction::get()->create_dataobject(handleList);

	pfc::com_ptr_t<com::IDropSourceImpl> pIDropSource = new com::IDropSourceImpl(
		wnd,
		pDO.get_ptr(),
		handleCount,
		parsedOptions.useTheming,
		parsedOptions.showText,
		parsedOptions.pCustomImage
	);

	SendMessageW(wnd, static_cast<UINT>(smp::InternalSyncMessage::wnd_internal_drag_start), 0, 0);

	DWORD returnEffect;
	HRESULT hr = SHDoDragDrop(nullptr, pDO.get_ptr(), pIDropSource.get_ptr(), okEffects, &returnEffect);

	SendMessageW(wnd, static_cast<UINT>(smp::InternalSyncMessage::wnd_internal_drag_stop), 0, 0);

	return (DRAGDROP_S_CANCEL == hr ? DROPEFFECT_NONE : returnEffect);
}

uint32_t Fb::DoDragDropWithOpt(size_t optArgCount, uint32_t hWnd, JsFbMetadbHandleList* handles, uint32_t okEffects, JS::HandleValue options)
{
	switch (optArgCount)
	{
	case 0:
		return DoDragDrop(hWnd, handles, okEffects, options);
	case 1:
		return DoDragDrop(hWnd, handles, okEffects);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void Fb::Exit()
{
	standard_commands::main_exit();
}

JSObject* Fb::GetAudioChunk(double requested_length, double offset)
{
	audio_chunk_impl chunk;
	double time{};

	if (vis_->get_absolute_time(time) && vis_->get_chunk_absolute(chunk, time + offset, requested_length))
	{
		return JsFbAudioChunk::CreateJs(pJsCtx_, chunk);
	}
	
	return nullptr;
}

JSObject* Fb::GetAudioChunkWithOpt(size_t optArgCount, double requested_length, double offset)
{
	switch (optArgCount)
	{
	case 0: return GetAudioChunk(requested_length, offset);
	case 1: return GetAudioChunk(requested_length);
	default: throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

JSObject* Fb::GetClipboardContents(uint32_t)
{
	const auto wnd = GetPanelHwndForCurrentGlobal(pJsCtx_);
	qwr::QwrException::ExpectTrue(wnd, "Method called before fb2k was initialized completely");

	auto api = ole_interaction::get();
	pfc::com_ptr_t<IDataObject> pDO;
	metadb_handle_list items;

	if (SUCCEEDED(OleGetClipboard(pDO.receive_ptr())))
	{
		DWORD drop_effect = DROPEFFECT_COPY;
		bool native;

		if (SUCCEEDED(api->check_dataobject(pDO, drop_effect, native)))
		{
			dropped_files_data_impl data;
			if (SUCCEEDED(api->parse_dataobject(pDO, data)))
			{
				data.to_handles(items, native, wnd);
			}
		}
	}

	return JsFbMetadbHandleList::CreateJs(pJsCtx_, items);
}

JSObject* Fb::GetClipboardContentsWithOpt(size_t optArgCount, uint32_t hWnd)
{
	switch (optArgCount)
	{
	case 0:
		return GetClipboardContents(hWnd);
	case 1:
		return GetClipboardContents();
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

std::string Fb::GetDSPPresets()
{
	auto j = JSON::array();
	auto api = dsp_config_manager_v2::get();

	const auto selectedPreset = api->get_selected_preset();
	pfc::string8 name;
	for (const auto i: ranges::views::indices(api->get_preset_count()))
	{
		api->get_preset_name(i, name);

		j.push_back({
			{ "active", selectedPreset == i },
			{ "name", name.get_ptr() }
		});
	}

	return j.dump(2);
}

JSObject* Fb::GetFocusItem(bool force)
{
	metadb_handle_ptr metadb;
	auto api = playlist_manager::get();

	if (!api->activeplaylist_get_focus_item_handle(metadb) && force)
	{
		api->activeplaylist_get_item_handle(metadb, 0);
	}

	if (metadb.is_empty())
	{
		return nullptr;
	}

	return JsFbMetadbHandle::CreateJs(pJsCtx_, metadb);
}

JSObject* Fb::GetFocusItemWithOpt(size_t optArgCount, bool force)
{
	switch (optArgCount)
	{
	case 0:
		return GetFocusItem(force);
	case 1:
		return GetFocusItem();
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

JSObject* Fb::GetLibraryItems()
{
	metadb_handle_list items;
	library_manager::get()->get_all_items(items);

	return JsFbMetadbHandleList::CreateJs(pJsCtx_, items);
}

pfc::string8 Fb::GetLibraryRelativePath(JsFbMetadbHandle* handle)
{
	qwr::QwrException::ExpectTrue(handle, "handle argument is null");

	pfc::string8 temp;
	library_manager::get()->get_relative_path(handle->GetHandle(), temp);
	return temp;
}

JSObject* Fb::GetNowPlaying()
{
	metadb_handle_ptr metadb;
	if (!playback_control::get()->get_now_playing(metadb))
	{
		return nullptr;
	}

	return JsFbMetadbHandle::CreateJs(pJsCtx_, metadb);
}

std::string Fb::GetOutputDevices()
{
	auto j = JSON::array();
	auto api = output_manager_v2::get();

	outputCoreConfig_t config{};
	api->getCoreConfig(config);

	api->listDevices([&j, &config](const std::string& name, const GUID& output_id, const GUID& device_id) {
		const std::string output_string = fmt::format("{{{}}}", pfc::print_guid(output_id).get_ptr());
		const std::string device_string = fmt::format("{{{}}}", pfc::print_guid(device_id).get_ptr());

		j.push_back({
			{ "name", name },
			{ "output_id", output_string },
			{ "device_id", device_string },
			{ "active", config.m_output == output_id && config.m_device == device_id }
		});
	});

	return j.dump(2);
}

JSObject* Fb::GetQueryItems(JsFbMetadbHandleList* handles, const std::string& query)
{
	qwr::QwrException::ExpectTrue(handles, "handles argument is null");

	const metadb_handle_list& handles_ptr = handles->GetHandleList();
	metadb_handle_list dst_list(handles_ptr);
	search_filter_v2::ptr filter;

	try
	{
		filter = search_filter_manager_v2::get()->create_ex(
			query.c_str(),
			fb2k::service_new<completion_notify_dummy>(),
			search_filter_manager_v2::KFlagSuppressNotify
		);
	}
	catch (const pfc::exception& e)
	{
		throw qwr::QwrException(e.what());
	}

	pfc::array_t<bool> mask;
	mask.set_size(dst_list.get_count());
	filter->test_multi(dst_list, mask.get_ptr());
	dst_list.filter_mask(mask.get_ptr());

	return JsFbMetadbHandleList::CreateJs(pJsCtx_, dst_list);
}

JSObject* Fb::GetSelection()
{
	metadb_handle_list items;
	ui_selection_manager::get()->get_selection(items);

	if (!items.get_count())
	{
		return nullptr;
	}

	return JsFbMetadbHandle::CreateJs(pJsCtx_, items[0]);
}

JSObject* Fb::GetSelections(uint32_t flags)
{
	metadb_handle_list items;
	ui_selection_manager_v2::get()->get_selection(items, flags);

	return JsFbMetadbHandleList::CreateJs(pJsCtx_, items);
}

JSObject* Fb::GetSelectionsWithOpt(size_t optArgCount, uint32_t flags)
{
	switch (optArgCount)
	{
	case 0:
		return GetSelections(flags);
	case 1:
		return GetSelections();
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

uint32_t Fb::GetSelectionType()
{
	const GUID type = ui_selection_manager_v2::get()->get_selection_type(0);
	const auto holderIdOpt = GetSelectionHolderTypeFromGuid(type);

	return holderIdOpt.value_or(0);
}

bool Fb::IsLibraryEnabled()
{
	return library_manager::get()->is_library_enabled();
}

bool Fb::IsMainMenuCommandChecked(const std::string& command)
{
	const auto status = utils::GetMainmenuCommandStatusByName(command);
	return (mainmenu_commands::flag_checked & status || mainmenu_commands::flag_radiochecked & status);
}

bool Fb::IsMetadbInMediaLibrary(JsFbMetadbHandle* handle)
{
	qwr::QwrException::ExpectTrue(handle, "handle argument is null");

	return library_manager::get()->is_item_in_library(handle->GetHandle());
}

void Fb::LoadPlaylist()
{
	standard_commands::main_load_playlist();
}

void Fb::Next()
{
	standard_commands::main_next();
}

void Fb::Pause()
{
	standard_commands::main_pause();
}

void Fb::Play()
{
	standard_commands::main_play();
}

void Fb::PlayOrPause()
{
	standard_commands::main_play_or_pause();
}

void Fb::Prev()
{
	standard_commands::main_previous();
}

void Fb::Random()
{
	standard_commands::main_random();
}

void Fb::RegisterMainMenuCommand(uint32_t id, const std::string& name, const std::optional<std::string>& description)
{
	const auto wnd = GetPanelHwndForCurrentGlobal(pJsCtx_);
	qwr::QwrException::ExpectTrue(wnd, "Method called before fb2k was initialized completely");

	DynamicMainMenuManager::Get().RegisterCommand(wnd, id, name, description);
}

void Fb::RegisterMainMenuCommandWithOpt(size_t optArgCount, uint32_t id, const std::string& name, const std::optional<std::string>& description)
{
	switch (optArgCount)
	{
	case 0:
		return RegisterMainMenuCommand(id, name, description);
	case 1:
		return RegisterMainMenuCommand(id, name);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void Fb::RequestBacklog(double time)
{
	vis_->request_backlog(time);
}

void Fb::Restart()
{
	standard_commands::main_restart();
}

bool Fb::RunContextCommand(const std::string& command, uint32_t flags)
{
	metadb_handle_list dummy_list;
	try
	{
		utils::ExecuteContextCommandByName(command, dummy_list, flags);
		return true;
	}
	catch (const qwr::QwrException&)
	{
		return false;
	}
}

bool Fb::RunContextCommandWithOpt(size_t optArgCount, const std::string& command, uint32_t flags)
{
	switch (optArgCount)
	{
	case 0:
		return RunContextCommand(command, flags);
	case 1:
		return RunContextCommand(command);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

bool Fb::RunContextCommandWithMetadb(const std::string& command, JS::HandleValue handle, uint32_t flags)
{
	qwr::QwrException::ExpectTrue(handle.isObject(), "handle argument is invalid");

	JS::RootedObject jsObject(pJsCtx_, &handle.toObject());

	auto* jsHandle = GetInnerInstancePrivate<JsFbMetadbHandle>(pJsCtx_, jsObject);
	auto* jsHandleList = GetInnerInstancePrivate<JsFbMetadbHandleList>(pJsCtx_, jsObject);
	qwr::QwrException::ExpectTrue(jsHandle || jsHandleList, "handle argument is invalid");

	metadb_handle_list handle_list;
	if (jsHandleList)
	{
		handle_list = jsHandleList->GetHandleList();
	}
	else
	{
		handle_list.add_item(jsHandle->GetHandle());
	}

	try
	{
		utils::ExecuteContextCommandByName(command, handle_list, flags);
		return true;
	}
	catch (const qwr::QwrException&)
	{
		return false;
	}
}

bool Fb::RunContextCommandWithMetadbWithOpt(size_t optArgCount, const std::string& command, JS::HandleValue handle, uint32_t flags)
{
	switch (optArgCount)
	{
	case 0:
		return RunContextCommandWithMetadb(command, handle, flags);
	case 1:
		return RunContextCommandWithMetadb(command, handle);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

bool Fb::RunMainMenuCommand(const std::string& command)
{
	try
	{
		utils::ExecuteMainmenuCommandByName(command);
		return true;
	}
	catch (qwr::QwrException&)
	{
		return false;
	}
}

void Fb::SavePlaylist()
{
	standard_commands::main_save_playlist();
}

void Fb::SetDSPPreset(uint32_t idx)
{
	auto api = dsp_config_manager_v2::get();
	t_size count = api->get_preset_count();

	qwr::QwrException::ExpectTrue(idx < count, "Index is out of bounds");

	api->select_preset(idx);
}

void Fb::SetOutputDevice(const std::wstring& output, const std::wstring& device)
{
	GUID output_id;
	GUID device_id;
	if (CLSIDFromString(output.c_str(), &output_id) == NOERROR && CLSIDFromString(device.c_str(), &device_id) == NOERROR)
	{
		output_manager_v2::get()->setCoreConfigDevice(output_id, device_id);
	}
}

void Fb::ShowConsole()
{
	constexpr GUID guid_main_show_console = { 0x5b652d25, 0xce44, 0x4737, { 0x99, 0xbb, 0xa3, 0xcf, 0x2a, 0xeb, 0x35, 0xcc } };
	standard_commands::run_main(guid_main_show_console);
}

void Fb::ShowLibrarySearchUI(const std::string& query)
{
	library_search_ui::get()->show(query.c_str());
}

void Fb::ShowPictureViewer(const std::wstring& image_path)
{
	auto data = AlbumArtStatic::to_data(image_path);
	AlbumArtStatic::show_viewer(data);
}

void Fb::ShowPopupMessage(const std::string& msg, const std::string& title)
{
	fb2k::inMainThread([msg, title] {
		popup_message::g_show(msg.c_str(), title.c_str());
	});
}

void Fb::ShowPopupMessageWithOpt(size_t optArgCount, const std::string& msg, const std::string& title)
{
	switch (optArgCount)
	{
	case 0:
		return ShowPopupMessage(msg, title);
	case 1:
		return ShowPopupMessage(msg);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void Fb::ShowPreferences()
{
	standard_commands::main_preferences();
}

void Fb::Stop()
{
	standard_commands::main_stop();
}

JSObject* Fb::TitleFormat(const std::string& expression)
{
	return JsFbTitleFormat::Constructor(pJsCtx_, expression);
}

void Fb::UnregisterMainMenuCommand(uint32_t id)
{
	const auto wnd = GetPanelHwndForCurrentGlobal(pJsCtx_);
	qwr::QwrException::ExpectTrue(wnd, "Method called before fb2k was initialized completely");

	DynamicMainMenuManager::Get().UnregisterCommand(wnd, id);
}

void Fb::VolumeDown()
{
	standard_commands::main_volume_down();
}

void Fb::VolumeMute()
{
	standard_commands::main_volume_mute();
}

void Fb::VolumeUp()
{
	standard_commands::main_volume_up();
}

bool Fb::get_AlwaysOnTop()
{
	return config_object::g_get_data_bool_simple(standard_config_objects::bool_ui_always_on_top, false);
}

std::string Fb::get_ComponentPath()
{
	return (smp::path::Component() / "").u8string();
}

bool Fb::get_CursorFollowPlayback()
{
	return config_object::g_get_data_bool_simple(standard_config_objects::bool_cursor_follows_playback, false);
}

int32_t Fb::get_CustomVolume()
{
	auto api = playback_control_v3::get();

	if (api->custom_volume_is_active())
	{
		return api->custom_volume_get();
	}

	return -1;
}

std::string Fb::get_FoobarPath()
{
	return (smp::path::Foobar2000() / "").u8string();
}

bool Fb::get_IsPaused()
{
	return playback_control::get()->is_paused();
}

bool Fb::get_IsPlaying()
{
	return playback_control::get()->is_playing();
}

bool Fb::get_PlaybackFollowCursor()
{
	return config_object::g_get_data_bool_simple(standard_config_objects::bool_playback_follows_cursor, false);
}

double Fb::get_PlaybackLength()
{
	return playback_control::get()->playback_get_length();
}

double Fb::get_PlaybackTime()
{
	return playback_control::get()->playback_get_position();
}

std::string Fb::get_ProfilePath()
{
	return (smp::path::Profile() / "").u8string();
}

uint32_t Fb::get_ReplaygainMode()
{
	t_replaygain_config rg;
	replaygain_manager::get()->get_core_settings(rg);
	return rg.m_source_mode;
}

bool Fb::get_StopAfterCurrent()
{
	return playback_control::get()->get_stop_after_current();
}

std::string Fb::get_Version()
{
	return core_version_info_v2::get()->get_version_as_text();
}

float Fb::get_Volume()
{
	return playback_control::get()->get_volume();
}

void Fb::put_AlwaysOnTop(bool p)
{
	config_object::g_set_data_bool(standard_config_objects::bool_ui_always_on_top, p);
}

void Fb::put_CursorFollowPlayback(bool p)
{
	config_object::g_set_data_bool(standard_config_objects::bool_cursor_follows_playback, p);
}

void Fb::put_PlaybackFollowCursor(bool p)
{
	config_object::g_set_data_bool(standard_config_objects::bool_playback_follows_cursor, p);
}

void Fb::put_PlaybackTime(double time)
{
	playback_control::get()->playback_seek(time);
}

void Fb::put_ReplaygainMode(uint32_t p)
{
	static constexpr std::array guids =
	{
		&standard_commands::guid_main_rg_disable,
		&standard_commands::guid_main_rg_set_track,
		&standard_commands::guid_main_rg_set_album,
		&standard_commands::guid_main_rg_byorder
	};

	qwr::QwrException::ExpectTrue(p < guids.size(), "Invalid replay gain mode: {}", p);

	standard_commands::run_main(*guids[p]);
	playback_control_v3::get()->restart();
}

void Fb::put_StopAfterCurrent(bool p)
{
	playback_control::get()->set_stop_after_current(p);
}

void Fb::put_Volume(float value)
{
	auto api = playback_control_v3::get();

	if (!api->custom_volume_is_active())
	{
		api->set_volume(value);
	}
}

Fb::DoDragDropOptions Fb::ParseDoDragDropOptions(JS::HandleValue options)
{
	DoDragDropOptions parsedoptions;
	if (!options.isNullOrUndefined())
	{
		qwr::QwrException::ExpectTrue(options.isObject(), "options argument is not an object");
		JS::RootedObject jsOptions(pJsCtx_, &options.toObject());

		parsedoptions.useTheming = GetOptionalProperty<bool>(pJsCtx_, jsOptions, "use_theming").value_or(true);
		parsedoptions.useAlbumArt = GetOptionalProperty<bool>(pJsCtx_, jsOptions, "use_album_art").value_or(true);
		parsedoptions.showText = GetOptionalProperty<bool>(pJsCtx_, jsOptions, "show_text").value_or(true);
		auto jsImage = GetOptionalProperty<JsGdiBitmap*>(pJsCtx_, jsOptions, "custom_image").value_or(nullptr);
		if (jsImage)
		{
			parsedoptions.pCustomImage = jsImage->GdiBitmap();
		}
	}

	return parsedoptions;
}

} // namespace mozjs
