#include <stdafx.h>
#include "fb_tooltip.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>

using namespace smp;

namespace
{
	using namespace mozjs;

	JSClassOps jsOps = {
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		JsFbTooltip::FinalizeJsObject,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};

	JSClass jsClass = {
		"FbTooltip",
		kDefaultClassFlags,
		&jsOps
	};

	MJS_DEFINE_JS_FN_FROM_NATIVE(Activate, JsFbTooltip::Activate)
	MJS_DEFINE_JS_FN_FROM_NATIVE(Deactivate, JsFbTooltip::Deactivate)
	MJS_DEFINE_JS_FN_FROM_NATIVE(GetDelayTime, JsFbTooltip::GetDelayTime)
	MJS_DEFINE_JS_FN_FROM_NATIVE(SetDelayTime, JsFbTooltip::SetDelayTime)
	MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(SetFont, JsFbTooltip::SetFont, JsFbTooltip::SetFontWithOpt, 2)
	MJS_DEFINE_JS_FN_FROM_NATIVE(SetMaxWidth, JsFbTooltip::SetMaxWidth)
	MJS_DEFINE_JS_FN_FROM_NATIVE(TrackPosition, JsFbTooltip::TrackPosition)

	constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
		{
			JS_FN("Activate", Activate, 0, kDefaultPropsFlags),
			JS_FN("Deactivate", Deactivate, 0, kDefaultPropsFlags),
			JS_FN("GetDelayTime", GetDelayTime, 1, kDefaultPropsFlags),
			JS_FN("SetDelayTime", SetDelayTime, 2, kDefaultPropsFlags),
			JS_FN("SetFont", SetFont, 1, kDefaultPropsFlags),
			JS_FN("SetMaxWidth", SetMaxWidth, 1, kDefaultPropsFlags),
			JS_FN("TrackPosition", TrackPosition, 2, kDefaultPropsFlags),
			JS_FS_END,
		});

	MJS_DEFINE_JS_FN_FROM_NATIVE(get_Text, JsFbTooltip::get_Text)
	MJS_DEFINE_JS_FN_FROM_NATIVE(put_Text, JsFbTooltip::put_Text)
	MJS_DEFINE_JS_FN_FROM_NATIVE(put_TrackActivate, JsFbTooltip::put_TrackActivate)

	constexpr auto jsProperties = std::to_array<JSPropertySpec>(
		{
			JS_PSGS("Text", get_Text, put_Text, kDefaultPropsFlags),
			JS_PSGS("TrackActivate", DummyGetter, put_TrackActivate, kDefaultPropsFlags),
			JS_PS_END,
		});
}

namespace mozjs
{
	const JSClass JsFbTooltip::JsClass = jsClass;
	const JSFunctionSpec* JsFbTooltip::JsFunctions = jsFunctions.data();
	const JSPropertySpec* JsFbTooltip::JsProperties = jsProperties.data();
	const JsPrototypeId JsFbTooltip::PrototypeId = JsPrototypeId::FbTooltip;

	JsFbTooltip::JsFbTooltip(JSContext* ctx, HWND parent_wnd)
		: m_ctx(ctx)
		, m_parent_wnd(parent_wnd)
		, m_buffer(TEXT(SMP_NAME))
	{
		m_ctrl.Create(m_parent_wnd);
		qwr::CheckWinApi(m_ctrl, "tooltip::Create");

		// Original position
		m_ctrl.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		// Set up tooltip information.
		// note: we need to have text here, otherwise tooltip will glitch out
		m_info = std::make_unique<CToolInfo>(TTF_IDISHWND | TTF_SUBCLASS, m_parent_wnd, (UINT_PTR)m_parent_wnd, nullptr, const_cast<wchar_t*>(m_buffer.c_str()));

		auto bRet = m_ctrl.AddTool(m_info.get());
		qwr::CheckWinApi(bRet, "tooltip::AddTool");
		m_ctrl.Activate(FALSE);
	}

	std::unique_ptr<JsFbTooltip> JsFbTooltip::CreateNative(JSContext* cx, HWND hParentWnd)
	{
		QwrException::ExpectTrue(hParentWnd, "Internal error: hParentWnd is null");
		return std::unique_ptr<JsFbTooltip>(new JsFbTooltip(cx, hParentWnd));
	}

	size_t JsFbTooltip::GetInternalSize(HWND /*hParentWnd*/)
	{
		return sizeof(LOGFONT) + sizeof(TOOLINFO);
	}

	void JsFbTooltip::PrepareForGc()
	{
		if (m_ctrl.IsWindow())
		{
			Deactivate();

			if (m_info)
			{
				m_ctrl.DelTool(m_info.get());
			}

			m_ctrl.DestroyWindow();
		}
	}

	void JsFbTooltip::Update()
	{
		if (m_font && m_ctrl.IsWindow())
		{
			const auto is_dark = ui_config_manager::g_is_dark_mode();
			SetWindowTheme(m_ctrl.m_hWnd, is_dark ? L"DarkMode_Explorer" : nullptr, nullptr);
			m_ctrl.SetFont(m_font.get(), FALSE);
		}
	}

	void JsFbTooltip::Activate()
	{
		m_ctrl.Activate(TRUE);
	}

	void JsFbTooltip::Deactivate()
	{
		m_ctrl.Activate(FALSE);
	}

	uint32_t JsFbTooltip::GetDelayTime(uint32_t type)
	{
		QwrException::ExpectTrue(type >= TTDT_AUTOMATIC && type <= TTDT_INITIAL, "Invalid delay type: {}", type);
		return m_ctrl.GetDelayTime(type);
	}

	void JsFbTooltip::SetDelayTime(uint32_t type, int32_t time)
	{
		QwrException::ExpectTrue(type >= TTDT_AUTOMATIC && type <= TTDT_INITIAL, "Invalid delay type: {}", type);
		m_ctrl.SetDelayTime(type, time);
	}

	void JsFbTooltip::SetFont(const std::wstring& name, uint32_t pxSize, uint32_t style)
	{
		if (name.empty())
			return;
	
		m_font.reset(CreateFontW(
			// from msdn: "< 0, The font mapper transforms this value into device units
			//             and matches its absolute value against the character height of the available fonts."
			-static_cast<int>(pxSize),
			0,
			0,
			0,
			(style & Gdiplus::FontStyleBold) ? FW_BOLD : FW_NORMAL,
			(style & Gdiplus::FontStyleItalic) ? TRUE : FALSE,
			(style & Gdiplus::FontStyleUnderline) ? TRUE : FALSE,
			(style & Gdiplus::FontStyleStrikeout) ? TRUE : FALSE,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE,
			name.c_str()));

		qwr::CheckWinApi(!!m_font, "CreateFont");
		Update();
	}

	void JsFbTooltip::SetFontWithOpt(size_t optArgCount, const std::wstring& name, uint32_t pxSize, uint32_t style)
	{
		switch (optArgCount)
		{
		case 0:
			return SetFont(name, pxSize, style);
		case 1:
			return SetFont(name, pxSize);
		case 2:
			return SetFont(name);
		default:
			throw QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
		}
	}

	void JsFbTooltip::SetMaxWidth(uint32_t width)
	{
		m_ctrl.SetMaxTipWidth(width);
	}

	void JsFbTooltip::TrackPosition(int x, int y)
	{
		POINT pt{ x, y };
		ClientToScreen(m_parent_wnd, &pt);
		m_ctrl.TrackPosition(pt.x, pt.y);
	}

	std::wstring JsFbTooltip::get_Text()
	{
		return m_buffer;
	}

	void JsFbTooltip::put_Text(const std::wstring& text)
	{
		m_buffer = text;
		m_info->lpszText = m_buffer.data();
		m_ctrl.SetToolInfo(m_info.get());
	}

	void JsFbTooltip::put_TrackActivate(bool activate)
	{
		if (activate)
		{
			m_info->uFlags |= TTF_TRACK | TTF_ABSOLUTE;
		}
		else
		{
			m_info->uFlags &= ~(TTF_TRACK | TTF_ABSOLUTE);
		}

		m_ctrl.TrackActivate(m_info.get(), activate);
	}
}
