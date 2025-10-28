#pragma once

class PlaylistLock : public playlist_lock
{
public:
	PlaylistLock(uint32_t mask);

	static bool add(size_t playlistIndex, uint32_t mask) noexcept;
	static bool is_my_lock(size_t playlistIndex) noexcept;
	static bool remove(size_t playlistIndex) noexcept;
	static void before_ui_init() noexcept;

	bool execute_default_action(size_t) noexcept final;
	bool query_items_add(size_t, const pfc::list_base_const_t<metadb_handle_ptr>&, const bit_array&) noexcept final;
	bool query_items_remove(const bit_array&, bool) noexcept final;
	bool query_items_reorder(const size_t*, size_t) noexcept final;
	bool query_item_replace(size_t, const metadb_handle_ptr&, const metadb_handle_ptr&) noexcept final;
	bool query_playlist_remove() noexcept final;
	bool query_playlist_rename(const char*, size_t) noexcept final;
	uint32_t get_filter_mask() noexcept final;
	void get_lock_name(pfc::string_base& out) noexcept final;

	void on_playlist_index_change(size_t) noexcept final {}
	void on_playlist_remove() noexcept final {}
	void show_ui() noexcept final {}

private:
	static inline pfc::map_t<GUID, playlist_lock::ptr> s_map;

	uint32_t m_mask{};
};
