#include <stdafx.h>

#include <libPPUI/gdiplus_helpers.h>

#include <2K3/PlaylistLock.hpp>
#include <config/delayed_package_utils.h>
#include <events/event_dispatcher.h>
#include <js_engine/js_engine.h>

#include <utils/thread_pool_instance.h>
#include <qwr/error_popup.h>

#include <Scintilla.h>

DECLARE_COMPONENT_VERSION(SMP_NAME, SMP_VERSION, about_smp().c_str());
VALIDATE_COMPONENT_FILENAME(SMP_DLL_NAME);

namespace smp::com
{
	ITypeLibPtr g_typelib;
}

namespace
{
	CAppModule wtl_module;
	GdiplusScope scope;
	HMODULE rich_edit_ctrl{};

	class InitStageCallback : public init_stage_callback
	{
		void on_init_stage(t_uint32 stage) noexcept final
		{
			if (stage == init_stages::before_config_read)
			{
				try
				{
					smp::config::ProcessDelayedPackages();
				}
				catch (const qwr::QwrException& e)
				{
					qwr::ReportErrorWithPopup(SMP_UNDERSCORE_NAME, fmt::format("Failed to process delayed packages:\n{}", e.what()));
				}
			}
			else if (stage == init_stages::before_ui_init)
			{
				PlaylistLock::before_ui_init();

				const auto ins = core_api::get_my_instance();

				Scintilla_RegisterClasses(ins);
				rich_edit_ctrl = LoadLibraryW(CRichEditCtrl::GetLibraryName());

				const auto path = wil::GetModuleFileNameW(ins);
				std::ignore = LoadTypeLibEx(path.get(), REGKIND_NONE, &smp::com::g_typelib);

				std::ignore = wtl_module.Init(nullptr, ins);
			}
		}
	};

	void on_quit() noexcept
	{
		mozjs::JsEngine::GetInstance().PrepareForExit();
		smp::EventDispatcher::Get().NotifyAllAboutExit();
		smp::GetThreadPoolInstance().Finalize();
		Scintilla_ReleaseResources();
		FreeLibrary(rich_edit_ctrl);
		wtl_module.Term();
		smp::com::g_typelib.Release();
	}

	FB2K_SERVICE_FACTORY(InitStageCallback);
	FB2K_RUN_ON_QUIT(on_quit);
}
