#include <stdafx.h>

#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>

namespace smp
{
	class LibraryCallback : public library_callback_v2
	{
	public:
		void on_items_added(metadb_handle_list_cref handles) noexcept final
		{
			if (library_manager_v4::get()->is_initialized())
			{
				EventDispatcher::Get().PutEventToAll(GenerateEvent_JsCallback(EventId::kFbLibraryItemsAdded, std::make_shared<metadb_handle_list>(handles)));
			}
		}

		void on_items_modified_v2(metadb_handle_list_cref handles, metadb_io_callback_v2_data&) noexcept final
		{
			if (library_manager_v4::get()->is_initialized())
			{
				EventDispatcher::Get().PutEventToAll(GenerateEvent_JsCallback(EventId::kFbLibraryItemsChanged, std::make_shared<metadb_handle_list>(handles), is_modified_from_hook()));
			}
		}

		void on_items_removed(metadb_handle_list_cref handles) noexcept final
		{
			if (library_manager_v4::get()->is_initialized())
			{
				EventDispatcher::Get().PutEventToAll(GenerateEvent_JsCallback(EventId::kFbLibraryItemsRemoved, std::make_shared<metadb_handle_list>(handles)));
			}
		}

		void on_library_initialized() noexcept final
		{
			fb2k::inMainThread([this]
				{
					metadb_handle_list items;
					library_manager::get()->get_all_items(items);
					on_items_added(items);
				});
		}

		void on_items_modified(metadb_handle_list_cref) noexcept final {}
	};

	FB2K_SERVICE_FACTORY(LibraryCallback);
}
