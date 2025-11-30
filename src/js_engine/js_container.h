#pragma once
#include "js_realm_inner.h"
#include "native_to_js_invoker.h"

namespace smp::panel
{
	class js_panel_window;
	struct DragActionParams;
}

namespace mozjs
{
	class JsAsyncTask;
	class JsDropSourceAction;
	class JsGdiGraphics;
	class JsGlobalObject;

	// Must not leak exceptions!
	class JsContainer final : public std::enable_shared_from_this<JsContainer>
	{
		friend class JsEngine;

	public:
		JsContainer(smp::panel::js_panel_window& parentPanel);
		JsContainer(const JsContainer&) = delete;
		~JsContainer();

	public:
		enum class JsStatus
		{
			EngineFailed,
			Failed,
			Ready,
			Working
		};

	public:
		[[nodiscard]] bool Initialize();
		void Finalize();

		void Fail(const std::string& errorText);

		[[nodiscard]] JsStatus GetStatus() const;

		[[nodiscard]] bool ExecuteScript(const std::string& scriptCode);
		[[nodiscard]] bool ExecuteScriptFile(const std::filesystem::path& scriptPath);

		static void RunJobs();

	public:
		smp::panel::js_panel_window& GetParentPanel() const;

	public:
		template <typename ReturnType = std::nullptr_t, typename... ArgTypes>
		std::optional<ReturnType> InvokeJsCallback(std::string functionName, ArgTypes&&... args)
		{
			if (!IsReadyForCallback())
			{
				return std::nullopt;
			}

			auto selfSaver = shared_from_this();

			OnJsActionStart();
			auto autoAction = wil::scope_exit([&] { OnJsActionEnd(); });

			return mozjs::InvokeJsCallback<ReturnType>(pJsCtx_, jsGlobal_, functionName, std::forward<ArgTypes>(args)...);
		}

		[[nodiscard]] bool InvokeOnDragAction(const std::string& functionName, const POINTL& pt, uint32_t keyState, smp::panel::DragActionParams& actionParams);
		void InvokeOnNotify(const std::wstring& name, JS::HandleValue info);
		void InvokeOnPaint(Gdiplus::Graphics& gr);
		bool InvokeJsAsyncTask(JsAsyncTask& jsTask);

	private:
		void SetJsCtx(JSContext* cx);

		[[nodiscard]] bool IsReadyForCallback() const;

		/// @return true on success, false with JS report on failure
		[[nodiscard]] bool CreateDropActionIfNeeded();

		void OnJsActionStart();
		void OnJsActionEnd();

	private:
		JSContext* pJsCtx_ = nullptr;
		smp::panel::js_panel_window* pParentPanel_ = nullptr;

		JS::PersistentRootedObject jsGlobal_;
		JS::PersistentRootedObject jsGraphics_;
		JS::PersistentRootedObject jsDropAction_;

		JsRealmInner* pNativeRealm_{};
		JsGlobalObject* pNativeGlobal_{};
		JsGdiGraphics* pNativeGraphics_{};
		JsDropSourceAction* pNativeDropAction_{};

		JsStatus jsStatus_ = JsStatus::EngineFailed;
		bool isParsingScript_ = false;
		uint32_t nestedJsCounter_ = 0;
	};
}
