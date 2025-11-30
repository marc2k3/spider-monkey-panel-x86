#include <stdafx.h>
#include "js_art_helpers.h"

#include <2K3/AlbumArtStatic.hpp>
#include <convert/native_to_js.h>
#include <events/event_dispatcher.h>
#include <events/event_js_task.h>
#include <interfaces/gdi_bitmap.h>
#include <js_objects/global_object.h>
#include <js_objects/global_heap_manager.h>
#include <js_utils/js_async_task.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/js_property_helper.h>
#include <qwr/thread_pool.h>
#include <utils/gdi_helpers.h>

using namespace smp;
using namespace mozjs;

namespace
{

class JsAlbumArtTask
	: public JsAsyncTaskImpl<JS::HandleValue>
{
public:
	JsAlbumArtTask(JSContext* cx,
					JS::HandleValue jsPromise);
	~JsAlbumArtTask() override = default;

	void SetData(std::unique_ptr<Gdiplus::Bitmap> image, const std::string& path);

private:
	bool InvokeJsImpl(JSContext* cx, JS::HandleObject jsGlobal, JS::HandleValue jsPromiseValue) override;

private:
	std::unique_ptr<Gdiplus::Bitmap> image_;
	std::string path_;
};

class AlbumArtV2FetchTask
{
public:
	AlbumArtV2FetchTask(
		JSContext* cx,
		JS::HandleObject jsPromise,
		HWND wnd,
		const metadb_handle_ptr& handle,
		uint32_t art_id,
		bool want_stub,
		bool only_embed
	);

	/// @details Executed off main thread
	~AlbumArtV2FetchTask() = default;

	AlbumArtV2FetchTask(const AlbumArtV2FetchTask&) = delete;
	AlbumArtV2FetchTask& operator=(const AlbumArtV2FetchTask&) = delete;

	/// @details Executed off main thread
	void operator()();

private:
	HWND m_wnd;
	metadb_handle_ptr m_handle;
	uint32_t m_art_id{};
	bool m_want_stub{}, m_only_embed{};

	std::shared_ptr<JsAlbumArtTask> jsTask_;
};

} // namespace

namespace
{

AlbumArtV2FetchTask::AlbumArtV2FetchTask(
	JSContext* cx,
	JS::HandleObject jsPromise,
	HWND wnd,
	const metadb_handle_ptr& handle,
	uint32_t art_id,
	bool want_stub,
	bool only_embed)
	: m_wnd(wnd)
	, m_handle(handle)
	, m_art_id(art_id)
	, m_want_stub(want_stub)
	, m_only_embed(only_embed)
{
	assert(cx);
	JS::RootedValue jsPromiseValue(cx, JS::ObjectValue(*jsPromise));
	jsTask_ = std::make_unique<JsAlbumArtTask>(cx, jsPromiseValue);
}

void AlbumArtV2FetchTask::operator()()
{
	if (!jsTask_->IsCanceled())
	{ // the task still might be executed and posted, since we don't block here
		return;
	}

	std::string imagePath;
	auto data = AlbumArtStatic::get(m_handle, m_art_id, m_want_stub, m_only_embed, imagePath);
	auto bitmap = AlbumArtStatic::to_bitmap(data);

	jsTask_->SetData(std::move(bitmap), imagePath);

	EventDispatcher::Get().PutEvent(m_wnd, std::make_unique<Event_JsTask>(EventId::kInternalGetAlbumArtPromiseDone, jsTask_));
}

JsAlbumArtTask::JsAlbumArtTask(JSContext* cx, JS::HandleValue jsPromise) : JsAsyncTaskImpl(cx, jsPromise) {}

void JsAlbumArtTask::SetData(std::unique_ptr<Gdiplus::Bitmap> image, const std::string& path)
{
	image_ = std::move(image);
	path_ = path;
}

bool JsAlbumArtTask::InvokeJsImpl(JSContext* cx, JS::HandleObject, JS::HandleValue jsPromiseValue)
{
	JS::RootedObject jsPromise(cx, &jsPromiseValue.toObject());

	try
	{
		JS::RootedValue jsBitmapValue(cx);
		if (image_)
		{
			JS::RootedObject jsBitmap(cx, JsGdiBitmap::CreateJs(cx, std::move(image_)));
			jsBitmapValue = jsBitmap ? JS::ObjectValue(*jsBitmap) : JS::UndefinedValue();
		}

		JS::RootedObject jsResult(cx, JS_NewPlainObject(cx));
		if (!jsResult)
		{
			throw JsException();
		}

		AddProperty(cx, jsResult, "image", JS::HandleValue{ jsBitmapValue });
		AddProperty(cx, jsResult, "path", path_);

		JS::RootedValue jsResultValue(cx, JS::ObjectValue(*jsResult));
		(void)JS::ResolvePromise(cx, jsPromise, jsResultValue);
	}
	catch (...)
	{
		mozjs::error::ExceptionToJsError(cx);

		JS::RootedValue jsError(cx);
		(void)JS_GetPendingException(cx, &jsError);

		JS::RejectPromise(cx, jsPromise, jsError);
	}

	return true;
}

} // namespace

namespace mozjs::art
{

JSObject* GetAlbumArtPromise(JSContext* cx, HWND hWnd, const metadb_handle_ptr& handle, uint32_t art_id, bool want_stub, bool only_embed)
{
	JS::RootedObject jsObject(cx, JS::NewPromiseObject(cx, nullptr));
	JsException::ExpectTrue(jsObject);

	QwrThreadPool::GetInstance().AddTask([task = std::make_shared<AlbumArtV2FetchTask>(cx, jsObject, hWnd, handle, art_id, want_stub, only_embed)]
		{
			std::invoke(*task);
		});

	return jsObject;
}

} // namespace mozjs::art
