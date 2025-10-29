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
	wil::com_ptr<ITypeLib> typelib;
}

namespace
{
	CAppModule wtl_module;
	GdiplusScope scope;
	wil::unique_hmodule rich_edit_ctrl;

	class InitStageCallback : public init_stage_callback
	{
		void on_init_stage(t_uint32 stage) noexcept final
		{
			if (stage == init_stages::before_config_read)
			{
				smp::config::ProcessDelayedPackages();
			}
			else if (stage == init_stages::before_ui_init)
			{
				const auto ins = core_api::get_my_instance();
				Scintilla_RegisterClasses(ins);
				PlaylistLock::before_ui_init();
				
				rich_edit_ctrl.reset(LoadLibraryW(CRichEditCtrl::GetLibraryName()));
				std::ignore = wtl_module.Init(nullptr, ins);

				const auto path = wil::GetModuleFileNameW(ins);
				std::ignore = LoadTypeLibEx(path.get(), REGKIND_NONE, &smp::com::typelib);
			}
		}
	};

	void on_quit() noexcept
	{
		mozjs::JsEngine::GetInstance().PrepareForExit();
		smp::EventDispatcher::Get().NotifyAllAboutExit();
		smp::GetThreadPoolInstance().Finalize();
		Scintilla_ReleaseResources();
		rich_edit_ctrl.reset();
		smp::com::typelib.reset();
		wtl_module.Term();
	}

	FB2K_SERVICE_FACTORY(InitStageCallback);
	FB2K_RUN_ON_QUIT(on_quit);
}
