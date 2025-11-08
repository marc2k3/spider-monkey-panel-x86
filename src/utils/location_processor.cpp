#include <stdafx.h>

#include "location_processor.h"

namespace smp::utils
{
	OnProcessLocationsNotify_InsertHandles::OnProcessLocationsNotify_InsertHandles(GUID g, UINT baseIdx, bool shouldSelect)
		: g_(g)
		, baseIdx_(baseIdx)
		, shouldSelect_(shouldSelect) {}

	void OnProcessLocationsNotify_InsertHandles::on_completion(metadb_handle_list_cref items)
	{
		auto api = playlist_manager_v5::get();
		const auto playlistIndex = api->find_playlist_by_guid(g_);

		if (playlistIndex == SIZE_MAX)
			return;

		const auto mask = api->playlist_lock_get_filter_mask(playlistIndex);

		if (WI_IsFlagSet(mask, playlist_lock::filter_add))
			return;

		pfc::bit_array_val selection(shouldSelect_);
		api->playlist_insert_items(playlistIndex, baseIdx_, items, selection);

		if (shouldSelect_)
		{
			api->set_active_playlist(playlistIndex);
			api->playlist_set_focus_item(playlistIndex, baseIdx_);
		}
	}
}
