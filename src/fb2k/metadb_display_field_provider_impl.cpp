#include "stdafx.h"
#include "stats.h"

namespace
{
	static constexpr std::array field_names =
	{
		"smp_playcount",
		"smp_loved",
		"smp_first_played",
		"smp_last_played",
		"smp_rating",
	};

	class metadb_display_field_provider_impl : public metadb_display_field_provider
	{
	public:
		bool process_field(uint32_t index, metadb_handle* handle, titleformat_text_out* out) final
		{
			metadb_index_hash hash;
			if (!smp::stats::HashHandle(handle, hash))
				return false;

			const auto f = smp::stats::GetStats(hash);

			switch (index)
			{
			case 0:
				if (f.playcount == 0)
					return false;

				out->write_int(titleformat_inputtypes::meta, f.playcount);
				return true;
			case 1:
				if (f.loved == 0)
					return false;

				out->write_int(titleformat_inputtypes::meta, f.loved);
				return true;
			case 2:
				if (f.first_played.empty())
					return false;

				out->write(titleformat_inputtypes::meta, f.first_played);
				return true;
			case 3:
				if (f.last_played.empty())
					return false;

				out->write(titleformat_inputtypes::meta, f.last_played);
				return true;
			case 4:
				if (f.rating == 0)
					return false;

				out->write_int(titleformat_inputtypes::meta, f.rating);
				return true;
			default:
				return false;
			}
		}

		uint32_t get_field_count() final
		{
			return static_cast<uint32_t>(field_names.size());
		}

		void get_field_name(uint32_t index, pfc::string_base& out) final
		{
			out = field_names[index];
		}
	};

	FB2K_SERVICE_FACTORY(metadb_display_field_provider_impl);
}
