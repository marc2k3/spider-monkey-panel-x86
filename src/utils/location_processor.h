#pragma once

namespace smp::utils
{
	class OnProcessLocationsNotify_InsertHandles : public process_locations_notify
	{
	public:
		OnProcessLocationsNotify_InsertHandles(GUID g, UINT baseIdx, bool shouldSelect);

		void on_completion(metadb_handle_list_cref items) final;
		void on_aborted() final {}

	private:
		const GUID g_;
		const size_t baseIdx_;
		const bool shouldSelect_;
	};
}
