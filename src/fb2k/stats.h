#pragma once

namespace smp::stats
{
	struct Fields
	{
		uint32_t playcount{};
		uint32_t loved{};
		pfc::string8 first_played;
		pfc::string8 last_played;
		uint32_t rating{};
	};

	[[nodiscard]] Fields GetStats(metadb_index_hash hash);
	[[nodiscard]] bool HashHandle(metadb_handle_ptr const& pMetadb, metadb_index_hash& hash);
	void RefreshStats(const metadb_index_hash& hash);
	void RefreshStats(const pfc::list_base_const_t<metadb_index_hash>& hashes);
	void SetStats(metadb_index_hash hash, const Fields& f);
}
