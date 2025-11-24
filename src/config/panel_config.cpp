#include <stdafx.h>
#include "panel_config.h"

#include <2K3/String.hpp>
#include <config/panel_config_binary.h>
#include <config/panel_config_json.h>
#include <resources/resource.h>
#include <utils/guid_helpers.h>

namespace smp::config
{
	enum class SettingsType : uint32_t
	{
		Binary = 1,
		Json = 2
	};

	PanelProperties PanelProperties::FromJson(const std::string& jsonString)
	{
		return smp::config::json::DeserializeProperties(jsonString);
	}

	std::string PanelProperties::ToJson() const
	{
		return smp::config::json::SerializeProperties(*this);
	}

	void PanelProperties::Save(stream_writer* writer, abort_callback& abort) const
	{
		smp::config::json::SaveProperties(writer, abort, *this);
	}

	std::string PanelSettings_InMemory::GetDefaultScript()
	{
		return get_resource_text(IDR_DEFAULT_SCRIPT);
	}

	PanelSettings::PanelSettings()
	{
		ResetToDefault();
	}

	void PanelSettings::ResetToDefault()
	{
		payload = PanelSettings_InMemory{};
		isPseudoTransparent = false;
		edgeStyle = EdgeStyle::NoEdge;
		id = [] {
			const auto guidStr = utils::GuidToStr(utils::GenerateGuid());
			return qwr::ToU8(guidStr);
		}();
	}

	PanelSettings PanelSettings::Load(stream_reader* reader, size_t size, abort_callback& abort)
	{
		if (size < sizeof(SettingsType))
		{
			return {};
		}

		try
		{
			const auto ver = reader->read_object_t<uint32_t>(abort);

			switch (static_cast<SettingsType>(ver))
			{
			case SettingsType::Binary:
				return smp::config::binary::LoadSettings(reader, abort);
			case SettingsType::Json:
				return smp::config::json::LoadSettings(reader, abort);
			default:
				throw QwrException("Unexpected panel settings format: {}", ver);
			}
		}
		catch (const pfc::exception& e)
		{
			throw QwrException(e.what());
		}
	}

	void PanelSettings::Save(stream_writer* writer, abort_callback& abort) const
	{
		writer->write_object_t(static_cast<uint32_t>(SettingsType::Json), abort);
		smp::config::json::SaveSettings(writer, abort, *this);
	}
}
