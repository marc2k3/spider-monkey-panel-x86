#pragma once

namespace mozjs
{
	class JsFbFileInfo : public JsObjectBase<JsFbFileInfo>
	{
	public:
		static constexpr bool HasProto = true;
		static constexpr bool HasGlobalProto = false;
		static constexpr bool HasProxy = false;
		static constexpr bool HasPostCreate = false;

		static const JSClass JsClass;
		static const JSFunctionSpec* JsFunctions;
		static const JSPropertySpec* JsProperties;
		static const JsPrototypeId PrototypeId;

	public:
		~JsFbFileInfo() override = default;

		static std::unique_ptr<JsFbFileInfo> CreateNative(JSContext* cx, metadb_info_container::ptr containerInfo);
		static size_t GetInternalSize(const metadb_info_container::ptr& containerInfo);

	public:
		int32_t InfoFind(const std::string& name);
		std::string InfoName(uint32_t index);
		std::string InfoValue(uint32_t index);
		int32_t MetaFind(const std::string& name);
		std::string MetaName(uint32_t index);
		std::string MetaValue(uint32_t index, uint32_t valueIndex);
		uint32_t MetaValueCount(uint32_t index);

	public:
		uint32_t get_InfoCount();
		uint32_t get_MetaCount();

	private:
		JsFbFileInfo(JSContext* cx, metadb_info_container::ptr containerInfo);

	private:
		[[maybe_unused]] JSContext* pJsCtx_ = nullptr;
		metadb_info_container::ptr containerInfo_;
		const file_info& fileInfo_;
	};
}
