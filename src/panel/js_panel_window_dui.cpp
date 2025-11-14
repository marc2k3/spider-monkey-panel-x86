#include <stdafx.h>
#include <panel/js_panel_window.h>
#include <utils/colour_helpers.h>

namespace smp::panel
{
	class js_panel_window_dui : public js_panel_window, public ui_element_instance, public CWindowImpl<js_panel_window_dui>
	{
#pragma region js_panel_window
	protected:
		DWORD GetColour(const GUID& guid, uint32_t) final
		{
			const auto colour = uiCallback_->query_std_color(guid);
			return smp::colour::ColorrefToArgb(colour);
		}

		HFONT GetFont(const GUID& guid, uint32_t) final
		{
			return uiCallback_->query_font_ex(guid);
		}

		void NotifySizeLimitChanged() final
		{
			uiCallback_->on_min_max_info_change();
		}
#pragma endregion

	public:
		js_panel_window_dui(ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback) : js_panel_window(PanelType::DUI), uiCallback_(callback)
		{
			set_configuration(cfg);
		}

#pragma region static
		static GUID g_get_guid()
		{
			return smp::guid::window_dui;
		}

		static GUID g_get_subclass()
		{
			return ui_element_subclass_utility;
		}

		static pfc::string8 g_get_description()
		{
			return "Customizable panel with JavaScript support.";
		}

		static ui_element_config::ptr g_get_default_configuration()
		{
			return ui_element_config::g_create_empty(g_get_guid());
		}

		static void g_get_name(pfc::string_base& out)
		{
			out = SMP_NAME;
		}
#pragma endregion

		BOOL ProcessWindowMessage(HWND wnd, uint32_t msg, WPARAM wp, LPARAM lp, LRESULT& lres, DWORD) override
		{
			switch (msg)
			{
			case WM_RBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONDBLCLK:
			case WM_CONTEXTMENU:
				if (uiCallback_->is_edit_mode_enabled())
				{
					return FALSE;
				}
				break;
			}

			lres = OnMessage(wnd, msg, wp, lp);
			return TRUE;
		}

		bool edit_mode_context_menu_test(const POINT&, bool) final
		{
			return true;
		}

		ui_element_config::ptr get_configuration() final
		{
			ui_element_config_builder builder;
			SaveSettings(&builder.m_stream, fb2k::noAbort);
			return builder.finish(g_get_guid());
		}

		void edit_mode_context_menu_build(const POINT& p_point, bool, HMENU p_menu, unsigned p_id_base) final
		{
			GenerateContextMenu(p_menu, p_point.x, p_point.y, p_id_base);
		}

		void edit_mode_context_menu_command(const POINT&, bool, unsigned p_id, unsigned p_id_base) final
		{
			ExecuteContextMenu(p_id, p_id_base);
		}

		void set_configuration(ui_element_config::ptr data) final
		{
			ui_element_config_parser parser(data);

			// FIX: If window already created, DUI won't destroy it and create it again.
			LoadSettings(&parser.m_stream, parser.get_remaining(), fb2k::noAbort, !!GetHWND());
		}

		void initialize_window(HWND parent)
		{
			Create(parent);
		}

	private:
		ui_element_instance_callback::ptr uiCallback_;
	};

	class impl : public ui_element_impl<js_panel_window_dui> {};

	FB2K_SERVICE_FACTORY(impl);
}
