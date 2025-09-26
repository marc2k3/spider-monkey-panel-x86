#include "stdafx.h"
#include "stats.h"

namespace
{
	class track_property_provider_impl : public track_property_provider_v2
	{
	public:
		bool is_our_tech_info(const char*) final
		{
			return false;
		}

		void enumerate_properties(metadb_handle_list_cref handles, track_property_callback& callback) final
		{
			const auto count = handles.get_count();

			if (count == 1)
			{
				metadb_index_hash hash;

				if (smp::stats::HashHandle(handles[0], hash))
				{
					auto f = smp::stats::GetStats(hash);

					if (f.playcount > 0)
						callback.set_property(SMP_NAME, 0.0, "Playcount", pfc::format_uint(f.playcount));

					if (f.loved > 0)
						callback.set_property(SMP_NAME, 1.0, "Loved", pfc::format_uint(f.loved));

					if (!f.first_played.empty())
						callback.set_property(SMP_NAME, 2.0, "First Played", f.first_played);

					if (!f.last_played.empty())
						callback.set_property(SMP_NAME, 3.0, "Last Played", f.last_played);

					if (f.rating > 0)
						callback.set_property(SMP_NAME, 4.0, "Rating", pfc::format_uint(f.rating));
				}
			}
			else
			{
				std::set<metadb_index_hash> hashes;
				uint32_t total{};

				for (const auto& handle : handles)
				{
					metadb_index_hash hash;

					if (smp::stats::HashHandle(handle, hash) && hashes.emplace(hash).second)
					{
						total += smp::stats::GetStats(hash).playcount;
					}
				}

				if (total > 0U)
				{
					callback.set_property(SMP_NAME, 0.0, "Playcount", pfc::format_uint(total));
				}
			}
		}

		void enumerate_properties_v2(metadb_handle_list_cref handles, track_property_callback_v2& callback) final
		{
			if (callback.is_group_wanted(SMP_NAME))
			{
				enumerate_properties(handles, callback);
			}
		}
	};

	FB2K_SERVICE_FACTORY(track_property_provider_impl);
}
