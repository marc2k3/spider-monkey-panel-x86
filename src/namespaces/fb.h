#pragma once

namespace mozjs
{
	class JsFbMetadbHandle;
	class JsFbMetadbHandleList;
	class JsGdiBitmap;

	class Fb : public JsObjectBase<Fb>
	{
	public:
		static constexpr bool HasProto = false;
		static constexpr bool HasProxy = false;
		static constexpr bool HasPostCreate = false;

		static const JSClass JsClass;
		static const JSFunctionSpec* JsFunctions;
		static const JSPropertySpec* JsProperties;

	public:
		~Fb() override = default;

		static std::unique_ptr<Fb> CreateNative(JSContext* ctx);
		static size_t GetInternalSize();

	public:
		JSObject* AcquireUiSelectionHolder();
		void AddDirectory();
		void AddFiles();
		uint32_t AddLocationsAsync(JS::HandleValue locations);
		bool CheckClipboardContents();
		void ClearPlaylist();
		bool CopyHandleListToClipboard(JsFbMetadbHandleList* handles);
		JSObject* CreateContextMenuManager();
		// TODO v2: remove
		JSObject* CreateHandleList();
		JSObject* CreateMainMenuManager();
		JSObject* CreateProfiler(const std::string& name = "");
		JSObject* CreateProfilerWithOpt(size_t optArgCount, const std::string& name);
		uint32_t DoDragDrop(uint32_t hWnd, JsFbMetadbHandleList* handles, uint32_t okEffects, JS::HandleValue options = JS::UndefinedHandleValue);
		uint32_t DoDragDropWithOpt(size_t optArgCount, uint32_t hWnd, JsFbMetadbHandleList* handles, uint32_t okEffects, JS::HandleValue options);
		void Exit();
		JSObject* GetAudioChunk(double requested_length, double offset = 0.0);
		JSObject* GetAudioChunkWithOpt(size_t optArgCount, double requested_length, double offset);
		JSObject* GetClipboardContents(uint32_t hWnd = 0);
		JSObject* GetClipboardContentsWithOpt(size_t optArgCount, uint32_t hWnd);
		std::string GetDSPPresets();
		JSObject* GetFocusItem(bool force = true);
		JSObject* GetFocusItemWithOpt(size_t optArgCount, bool force);
		JSObject* GetLibraryItems();
		pfc::string8 GetLibraryRelativePath(JsFbMetadbHandle* handle);
		JSObject* GetNowPlaying();
		std::string GetOutputDevices();
		JSObject* GetQueryItems(JsFbMetadbHandleList* handles, const std::string& query);
		JSObject* GetSelection();
		JSObject* GetSelections(uint32_t flags = 0);
		JSObject* GetSelectionsWithOpt(size_t optArgCount, uint32_t flags);
		uint32_t GetSelectionType();
		bool IsLibraryEnabled();
		bool IsMainMenuCommandChecked(const std::string& command);
		bool IsMetadbInMediaLibrary(JsFbMetadbHandle* handle);
		void LoadPlaylist();
		void Next();
		void Pause();
		void Play();
		void PlayOrPause();
		void Prev();
		void Random();
		void RegisterMainMenuCommand(uint32_t id, const std::string& name, const std::optional<std::string>& description = std::nullopt);
		void RegisterMainMenuCommandWithOpt(size_t optArgCount, uint32_t id, const std::string& name, const std::optional<std::string>& description);
		void RequestBacklog(double time);
		void Restart();
		bool RunContextCommand(const std::string& command, uint32_t flags = 0);
		bool RunContextCommandWithOpt(size_t optArgCount, const std::string& command, uint32_t flags);
		bool RunContextCommandWithMetadb(const std::string& command, JS::HandleValue handle, uint32_t flags = 0);
		bool RunContextCommandWithMetadbWithOpt(size_t optArgCount, const std::string& command, JS::HandleValue handle, uint32_t flags);
		bool RunMainMenuCommand(const std::string& command);
		void SavePlaylist();
		void SetDSPPreset(uint32_t idx);
		void SetOutputDevice(const std::wstring& output, const std::wstring& device);
		void ShowConsole();
		void ShowLibrarySearchUI(const std::string& query);
		void ShowPictureViewer(const std::wstring& image_path);
		void ShowPopupMessage(const std::string& msg, const std::string& title = "Spider Monkey Panel");
		void ShowPopupMessageWithOpt(size_t optArgCount, const std::string& msg, const std::string& title);
		void ShowPreferences();
		void Stop();
		JSObject* TitleFormat(const std::string& expression);
		void UnregisterMainMenuCommand(uint32_t id);
		void VolumeDown();
		void VolumeMute();
		void VolumeUp();

	public:
		bool get_AlwaysOnTop();
		std::string get_ComponentPath();
		bool get_CursorFollowPlayback();
		int32_t get_CustomVolume();
		std::string get_FoobarPath();
		bool get_IsPaused();
		bool get_IsPlaying();
		bool get_PlaybackFollowCursor();
		double get_PlaybackLength();
		double get_PlaybackTime();
		std::string get_ProfilePath();
		uint32_t get_ReplaygainMode();
		bool get_StopAfterCurrent();
		std::string get_Version();
		float get_Volume();
		void put_AlwaysOnTop(bool p);
		void put_CursorFollowPlayback(bool p);
		void put_PlaybackFollowCursor(bool p);
		void put_PlaybackTime(double time);
		void put_ReplaygainMode(uint32_t p);
		void put_StopAfterCurrent(bool p);
		void put_Volume(float value);

	private:
		Fb(JSContext* ctx);

		struct DoDragDropOptions
		{
			bool show_album_art = true;
			bool show_text = true;
			Gdiplus::Bitmap* custom_image = nullptr;
		};

		DoDragDropOptions ParseDoDragDropOptions(JS::HandleValue options);

	private:
		JSContext* m_ctx{};
		visualisation_stream_v2::ptr vis_;
	};
}
