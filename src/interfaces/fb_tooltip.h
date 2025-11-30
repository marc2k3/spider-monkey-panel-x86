#pragma once
#include <utils/gdi_helpers.h>

namespace mozjs
{
	class JsFbTooltip : public JsObjectBase<JsFbTooltip>
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
		// @remark No need to cleanup JS here, since it must be performed manually beforehand anyway
		~JsFbTooltip() override = default;

		static std::unique_ptr<JsFbTooltip> CreateNative(JSContext* ctx, HWND parent_wnd);
		static size_t GetInternalSize(HWND parent_wnd);

		void PrepareForGc();
		void Update();

	public:
		void Activate();
		void Deactivate();
		uint32_t GetDelayTime(uint32_t type);
		void SetDelayTime(uint32_t type, int32_t time);
		void SetFont(const std::wstring& name, uint32_t pxSize = 12, uint32_t style = 0);
		void SetFontWithOpt(size_t optArgCount, const std::wstring& name, uint32_t pxSize, uint32_t style);
		void SetMaxWidth(uint32_t width);
		void TrackPosition(int x, int y);

	public:
		std::wstring get_Text();
		void put_Text(const std::wstring& text);
		void put_TrackActivate(bool activate);

	private:
		JsFbTooltip(JSContext* ctx, HWND hParentWnd);

	private:
		JSContext* m_ctx;

		CToolTipCtrl m_ctrl;
		HWND m_parent_wnd{};
		wil::unique_hfont m_font;
		std::unique_ptr<CToolInfo> m_info;
		std::wstring m_buffer;
	};
}
