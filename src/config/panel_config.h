#pragma once

#include <js_utils/serialized_value.h>

namespace smp::config
{

enum class EdgeStyle : uint8_t
{
	NoEdge = 0,
	SunkenEdge,
	GreyEdge,
	Default = NoEdge,
};

struct PanelProperties
{
	using PropertyMap = std::unordered_map<std::wstring, std::shared_ptr<mozjs::SerializedJsValue>>;
	PropertyMap values;

public:
	/// @throw QwrException
	[[nodiscard]] static PanelProperties FromJson(const std::string& jsonString);

	/// @throw QwrException
	[[nodiscard]] std::string ToJson() const;

	/// @throw QwrException
	void Save(stream_writer* writer, abort_callback& abort) const;
};

struct PanelSettings_InMemory
{
	std::string script = GetDefaultScript();
	bool shouldGrabFocus = true;
	bool enableDragDrop = false;

	[[nodiscard]] static std::string GetDefaultScript();
};

struct PanelSettings_File
{
	std::string path;
};

struct PanelSettings_Sample
{
	std::string sampleName;
};

struct PanelSettings_Package
{
	std::string id;      ///< unique package id
	std::string name;    ///< used for logging only
	std::string author;  ///< used for logging only
	std::string version; ///< used for logging only
};

struct PanelSettings
{
	std::string id;
	EdgeStyle edgeStyle;
	bool isPseudoTransparent;
	PanelProperties properties;

	using ScriptVariant = std::variant<PanelSettings_InMemory, PanelSettings_File, PanelSettings_Sample, PanelSettings_Package>;
	ScriptVariant payload;

public:
	PanelSettings();

	void ResetToDefault();

	/// @throw QwrException
	[[nodiscard]] static PanelSettings Load(stream_reader* reader, size_t size, abort_callback& abort);

	/// @throw QwrException
	void Save(stream_writer* writer, abort_callback& abort) const;
};

} // namespace smp::config
