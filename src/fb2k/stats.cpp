#include <stdafx.h>
#include "stats.h"
#include <utils/logging.h>

namespace
{
	metadb_index_manager::ptr GetIndexManagerInstance()
	{
		static metadb_index_manager* cached = metadb_index_manager::get().detach();
		return cached;
	}

	class metadb_index_client_impl : public metadb_index_client
	{
	public:
		metadb_index_hash transform(const file_info& info, const playable_location& location) final
		{
			if (titleFormat_.is_empty())
			{
				titleformat_compiler::get()->compile_force(titleFormat_, "$lower(%artist% - %title%)");
			}

			pfc::string_formatter str;
			titleFormat_->run_simple(location, &info, str);
			return hasher_md5::get()->process_single_string(str).xorHalve();
		}

	private:
		titleformat_object::ptr titleFormat_;
	};

	metadb_index_client* g_client = new service_impl_single_t<metadb_index_client_impl>;

	void before_config_read()
	{
		auto api = GetIndexManagerInstance();

		try
		{
			api->add(g_client, smp::guid::metadb_index, system_time_periods::week * 4);
			api->dispatch_global_refresh();
		}
		catch (const std::exception& e)
		{
			api->remove(smp::guid::metadb_index);
			smp::utils::LogError(fmt::format("Stats initialization failed:\n {}", e.what()));
		}
	}

	FB2K_ON_INIT_STAGE(before_config_read, init_stages::before_config_read)
} // namespace

namespace smp::stats
{
	Fields GetStats(metadb_index_hash hash)
	{
		mem_block_container_impl temp;
		GetIndexManagerInstance()->get_user_data(smp::guid::metadb_index, hash, temp);

		if (temp.get_size())
		{
			try
			{
				auto reader = stream_reader_formatter_simple_ref(temp.get_ptr(), temp.get_size());

				Fields ret;
				reader >> ret.playcount;
				reader >> ret.loved;
				reader >> ret.first_played;
				reader >> ret.last_played;
				reader >> ret.rating;
				return ret;
			}
			catch (const exception_io_data&) {}
		}

		return {};
	}

	bool HashHandle(metadb_handle_ptr const& pMetadb, metadb_index_hash& hash)
	{
		return g_client->hashHandle(pMetadb, hash);
	}

	void RefreshStats(const metadb_index_hash& hash)
	{
		GetIndexManagerInstance()->dispatch_refresh(smp::guid::metadb_index, hash);
	}

	void RefreshStats(const pfc::list_base_const_t<metadb_index_hash>& hashes)
	{
		GetIndexManagerInstance()->dispatch_refresh(smp::guid::metadb_index, hashes);
	}

	void SetStats(metadb_index_hash hash, const Fields& f)
	{
		stream_writer_formatter_simple writer;
		writer << f.playcount;
		writer << f.loved;
		writer << f.first_played;
		writer << f.last_played;
		writer << f.rating;
		GetIndexManagerInstance()->set_user_data(smp::guid::metadb_index, hash, writer.m_buffer.get_ptr(), writer.m_buffer.get_size());
	}
}
