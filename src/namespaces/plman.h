#pragma once
#include <js_objects/object_base.h>

namespace mozjs
{
	class JsFbMetadbHandle;
	class JsFbMetadbHandleList;
	class JsFbPlayingItemLocation;
	class JsFbPlaylistRecycler;

	class Plman : public JsObjectBase<Plman>
	{
	public:
		static constexpr bool HasProto = false;
		static constexpr bool HasProxy = false;
		static constexpr bool HasPostCreate = false;

		static const JSClass JsClass;
		static const JSFunctionSpec* JsFunctions;
		static const JSPropertySpec* JsProperties;

	public:
		~Plman() override;

		static std::unique_ptr<Plman> CreateNative(JSContext* ctx);
		static size_t GetInternalSize();

		void PrepareForGc();

	public:
		void AddItemToPlaybackQueue(JsFbMetadbHandle* handle);
		void AddLocations(uint32_t playlistIndex, JS::HandleValue locations, bool select = false);
		void AddLocationsWithOpt(size_t optArgCount, uint32_t playlistIndex, JS::HandleValue locations, bool select);
		void AddPlaylistItemToPlaybackQueue(uint32_t playlistIndex, uint32_t playlistItemIndex);
		void ClearPlaylist(uint32_t playlistIndex);
		void ClearPlaylistSelection(uint32_t playlistIndex);
		uint32_t CreateAutoPlaylist(uint32_t playlistIndex, const std::string& name, const std::string& query, const std::string& sort = "", uint32_t flags = 0);
		uint32_t CreateAutoPlaylistWithOpt(size_t optArgCount, uint32_t playlistIndex, const std::string& name, const std::string& query, const std::string& sort, uint32_t flags);
		uint32_t CreatePlaylist(uint32_t playlistIndex, const std::string& name);
		uint32_t DuplicatePlaylist(uint32_t from, const std::string& name = "");
		uint32_t DuplicatePlaylistWithOpt(size_t optArgCount, uint32_t from, const std::string& name);
		void EnsurePlaylistItemVisible(uint32_t playlistIndex, uint32_t playlistItemIndex);
		bool ExecutePlaylistDefaultAction(uint32_t playlistIndex, uint32_t playlistItemIndex);
		int32_t FindByGUID(const std::string& str);
		uint32_t FindOrCreatePlaylist(const std::string& name, bool unlocked);
		int32_t FindPlaybackQueueItemIndex(JsFbMetadbHandle* handle, uint32_t playlistIndex, uint32_t playlistItemIndex);
		int32_t FindPlaylist(const std::string& name);
		void FlushPlaybackQueue();
		std::string GetGUID(uint32_t playlistIndex);
		JS::Value GetPlaybackQueueContents();
		JSObject* GetPlaybackQueueHandles();
		JSObject* GetPlayingItemLocation();
		int32_t GetPlaylistFocusItemIndex(uint32_t playlistIndex);
		JSObject* GetPlaylistItems(uint32_t playlistIndex);
		std::optional<pfc::string8> GetPlaylistLockName(uint32_t playlistIndex);
		JS::Value GetPlaylistLockedActions(uint32_t playlistIndex);
		pfc::string8 GetPlaylistName(uint32_t playlistIndex);
		JSObject* GetPlaylistSelectedItems(uint32_t playlistIndex);
		void InsertPlaylistItems(uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select = false);
		void InsertPlaylistItemsWithOpt(size_t optArgCount, uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select);
		void InsertPlaylistItemsFilter(uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select = false);
		void InsertPlaylistItemsFilterWithOpt(size_t optArgCount, uint32_t playlistIndex, uint32_t base, JsFbMetadbHandleList* handles, bool select);
		bool IsAutoPlaylist(uint32_t playlistIndex);
		bool IsPlaylistItemSelected(uint32_t playlistIndex, uint32_t playlistItemIndex);
		// TODO v2: remove
		bool IsPlaylistLocked(uint32_t playlistIndex);
		bool IsRedoAvailable(uint32_t playlistIndex);
		bool IsUndoAvailable(uint32_t playlistIndex);
		bool MovePlaylist(uint32_t from, uint32_t to);
		bool MovePlaylistSelection(uint32_t playlistIndex, int32_t delta);
		uint32_t PlaylistItemCount(uint32_t playlistIndex);
		void Redo(uint32_t playlistIndex);
		void RemoveItemFromPlaybackQueue(uint32_t index);
		void RemoveItemsFromPlaybackQueue(JS::HandleValue affectedItems);
		bool RemovePlaylist(uint32_t playlistIndex);
		void RemovePlaylistSelection(uint32_t playlistIndex, bool crop = false);
		void RemovePlaylistSelectionWithOpt(size_t optArgCount, uint32_t playlistIndex, bool crop);
		bool RemovePlaylistSwitch(uint32_t playlistIndex);
		bool RenamePlaylist(uint32_t playlistIndex, const std::string& name);
		void SetActivePlaylistContext();
		void SetPlaylistFocusItem(uint32_t playlistIndex, uint32_t playlistItemIndex);
		void SetPlaylistFocusItemByHandle(uint32_t playlistIndex, JsFbMetadbHandle* handle);
		void SetPlaylistLockedActions(uint32_t playlistIndex, JS::HandleValue lockedActions = JS::NullHandleValue);
		void SetPlaylistLockedActionsWithOpt(size_t optArgCount, uint32_t playlistIndex, JS::HandleValue lockedActions);
		void SetPlaylistSelection(uint32_t playlistIndex, JS::HandleValue affectedItems, bool state);
		void SetPlaylistSelectionSingle(uint32_t playlistIndex, uint32_t playlistItemIndex, bool state);
		bool ShowAutoPlaylistUI(uint32_t playlistIndex);
		bool SortByFormat(uint32_t playlistIndex, const std::string& pattern, bool selOnly = false);
		bool SortByFormatWithOpt(size_t optArgCount, uint32_t playlistIndex, const std::string& pattern, bool selOnly);
		bool SortByFormatV2(uint32_t playlistIndex, const std::string& pattern, int8_t direction = 1);
		bool SortByFormatV2WithOpt(size_t optArgCount, uint32_t playlistIndex, const std::string& pattern, int8_t direction);
		void SortPlaylistsByName(int8_t direction = 1);
		void SortPlaylistsByNameWithOpt(size_t optArgCount, int8_t direction);
		void Undo(uint32_t playlistIndex);
		void UndoBackup(uint32_t playlistIndex);

	public:
		int32_t get_ActivePlaylist();
		uint32_t get_PlaybackOrder();
		int32_t get_PlayingPlaylist();
		uint32_t get_PlaylistCount();
		JSObject* get_PlaylistRecycler();
		void put_ActivePlaylist(uint32_t playlistIndex);
		void put_PlaybackOrder(uint32_t order);
		void put_PlayingPlaylist(uint32_t playlistIndex);

	private:
		Plman(JSContext* ctx);

	private:
		static inline const std::unordered_map<std::string, uint32_t> s_actionToMask = {
			{ "AddItems", playlist_lock::filter_add },
			{ "RemoveItems", playlist_lock::filter_remove },
			{ "ReorderItems", playlist_lock::filter_reorder },
			{ "ReplaceItems", playlist_lock::filter_replace },
			{ "RenamePlaylist", playlist_lock::filter_rename },
			{ "RemovePlaylist", playlist_lock::filter_remove_playlist },
			{ "ExecuteDefaultAction", playlist_lock::filter_default_action }
		};

		JSContext* m_ctx{};
		JS::PersistentRootedObject m_recycler;
		playlist_manager_v5::ptr m_api;
	};
}
