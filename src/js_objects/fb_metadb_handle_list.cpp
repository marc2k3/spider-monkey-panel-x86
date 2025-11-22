#include <stdafx.h>
#include "fb_metadb_handle_list.h"

#include <2K3/Attach.hpp>
#include <2K3/CustomSort.hpp>
#include <2K3/TagWriter.hpp>
#include <fb2k/stats.h>
#include <js_engine/js_to_native_invoker.h>
#include <js_objects/fb_metadb_handle.h>
#include <js_objects/fb_metadb_handle_list_iterator.h>
#include <js_objects/fb_title_format.h>
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
	JsFbMetadbHandleList::FinalizeJsObject,
	nullptr,
	nullptr,
	nullptr,
	nullptr
};

JSClass jsClass = {
	"FbMetadbHandleList",
	kDefaultClassFlags,
	&jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE(Add, JsFbMetadbHandleList::Add);
MJS_DEFINE_JS_FN_FROM_NATIVE(AddRange, JsFbMetadbHandleList::AddRange);
MJS_DEFINE_JS_FN_FROM_NATIVE(AttachImage, JsFbMetadbHandleList::AttachImage);
MJS_DEFINE_JS_FN_FROM_NATIVE(BSearch, JsFbMetadbHandleList::BSearch);
MJS_DEFINE_JS_FN_FROM_NATIVE(CalcTotalDuration, JsFbMetadbHandleList::CalcTotalDuration);
MJS_DEFINE_JS_FN_FROM_NATIVE(CalcTotalSize, JsFbMetadbHandleList::CalcTotalSize);
MJS_DEFINE_JS_FN_FROM_NATIVE(Clone, JsFbMetadbHandleList::Clone);
MJS_DEFINE_JS_FN_FROM_NATIVE(Convert, JsFbMetadbHandleList::Convert);
MJS_DEFINE_JS_FN_FROM_NATIVE(RemoveAttachedImage, JsFbMetadbHandleList::RemoveAttachedImage);
MJS_DEFINE_JS_FN_FROM_NATIVE(RemoveAttachedImages, JsFbMetadbHandleList::RemoveAttachedImages);
MJS_DEFINE_JS_FN_FROM_NATIVE(Find, JsFbMetadbHandleList::Find);
MJS_DEFINE_JS_FN_FROM_NATIVE(GetLibraryRelativePaths, JsFbMetadbHandleList::GetLibraryRelativePaths);
MJS_DEFINE_JS_FN_FROM_NATIVE(Insert, JsFbMetadbHandleList::Insert);
MJS_DEFINE_JS_FN_FROM_NATIVE(InsertRange, JsFbMetadbHandleList::InsertRange);
MJS_DEFINE_JS_FN_FROM_NATIVE(MakeDifference, JsFbMetadbHandleList::MakeDifference);
MJS_DEFINE_JS_FN_FROM_NATIVE(MakeIntersection, JsFbMetadbHandleList::MakeIntersection);
MJS_DEFINE_JS_FN_FROM_NATIVE(MakeUnion, JsFbMetadbHandleList::MakeUnion);
MJS_DEFINE_JS_FN_FROM_NATIVE(OptimiseFileLayout, JsFbMetadbHandleList::OptimiseFileLayout);
MJS_DEFINE_JS_FN_FROM_NATIVE(OrderByFormat, JsFbMetadbHandleList::OrderByFormat);
MJS_DEFINE_JS_FN_FROM_NATIVE(OrderByPath, JsFbMetadbHandleList::OrderByPath);
MJS_DEFINE_JS_FN_FROM_NATIVE(OrderByRelativePath, JsFbMetadbHandleList::OrderByRelativePath);
MJS_DEFINE_JS_FN_FROM_NATIVE(RefreshStats, JsFbMetadbHandleList::RefreshStats);
MJS_DEFINE_JS_FN_FROM_NATIVE(Remove, JsFbMetadbHandleList::Remove);
MJS_DEFINE_JS_FN_FROM_NATIVE(RemoveAll, JsFbMetadbHandleList::RemoveAll);
MJS_DEFINE_JS_FN_FROM_NATIVE(RemoveById, JsFbMetadbHandleList::RemoveById);
MJS_DEFINE_JS_FN_FROM_NATIVE(RemoveRange, JsFbMetadbHandleList::RemoveRange);
MJS_DEFINE_JS_FN_FROM_NATIVE(SaveAs, JsFbMetadbHandleList::SaveAs);
MJS_DEFINE_JS_FN_FROM_NATIVE(Sort, JsFbMetadbHandleList::Sort);
MJS_DEFINE_JS_FN_FROM_NATIVE(UpdateFileInfoFromJSON, JsFbMetadbHandleList::UpdateFileInfoFromJSON);

MJS_DEFINE_JS_FN_FROM_NATIVE(CreateIterator, JsFbMetadbHandleList::CreateIterator);

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
	{
		JS_FN("Add", Add, 1, kDefaultPropsFlags),
		JS_FN("AddRange", AddRange, 1, kDefaultPropsFlags),
		JS_FN("AttachImage", AttachImage, 2, kDefaultPropsFlags),
		JS_FN("BSearch", BSearch, 1, kDefaultPropsFlags),
		JS_FN("CalcTotalDuration", CalcTotalDuration, 0, kDefaultPropsFlags),
		JS_FN("CalcTotalSize", CalcTotalSize, 0, kDefaultPropsFlags),
		JS_FN("Clone", Clone, 0, kDefaultPropsFlags),
		JS_FN("Convert", Convert, 0, kDefaultPropsFlags),
		JS_FN("Find", Find, 1, kDefaultPropsFlags),
		JS_FN("GetLibraryRelativePaths", GetLibraryRelativePaths, 0, kDefaultPropsFlags),
		JS_FN("Insert", Insert, 2, kDefaultPropsFlags),
		JS_FN("InsertRange", InsertRange, 2, kDefaultPropsFlags),
		JS_FN("MakeDifference", MakeDifference, 1, kDefaultPropsFlags),
		JS_FN("MakeIntersection", MakeIntersection, 1, kDefaultPropsFlags),
		JS_FN("MakeUnion", MakeUnion, 1, kDefaultPropsFlags),
		JS_FN("OptimiseFileLayout", OptimiseFileLayout, 1, kDefaultPropsFlags),
		JS_FN("OrderByFormat", OrderByFormat, 2, kDefaultPropsFlags),
		JS_FN("OrderByPath", OrderByPath, 0, kDefaultPropsFlags),
		JS_FN("OrderByRelativePath", OrderByRelativePath, 0, kDefaultPropsFlags),
		JS_FN("RefreshStats", RefreshStats, 0, kDefaultPropsFlags),
		JS_FN("Remove", Remove, 1, kDefaultPropsFlags),
		JS_FN("RemoveAll", RemoveAll, 0, kDefaultPropsFlags),
		JS_FN("RemoveAttachedImage", RemoveAttachedImage, 1, kDefaultPropsFlags),
		JS_FN("RemoveAttachedImages", RemoveAttachedImages, 0, kDefaultPropsFlags),
		JS_FN("RemoveById", RemoveById, 1, kDefaultPropsFlags),
		JS_FN("RemoveRange", RemoveRange, 2, kDefaultPropsFlags),
		JS_FN("SaveAs", SaveAs, 1, kDefaultPropsFlags),
		JS_FN("Sort", Sort, 0, kDefaultPropsFlags),
		JS_FN("UpdateFileInfoFromJSON", UpdateFileInfoFromJSON, 1, kDefaultPropsFlags),
		JS_SYM_FN(iterator, CreateIterator, 0, kDefaultPropsFlags),
		JS_FS_END,
	});

MJS_DEFINE_JS_FN_FROM_NATIVE(get_Count, JsFbMetadbHandleList::get_Count);

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
	{
		JS_PSG("Count", get_Count, kDefaultPropsFlags),
		JS_PS_END,
	});

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT(FbMetadbHandleList_Constructor, JsFbMetadbHandleList::Constructor, JsFbMetadbHandleList::ConstructorWithOpt, 1)

} // namespace

namespace
{

// Wrapper to intercept indexed gets/sets.
class FbMetadbHandleListProxyHandler : public js::ForwardingProxyHandler
{
public:
	static const FbMetadbHandleListProxyHandler singleton;

	FbMetadbHandleListProxyHandler()
		: js::ForwardingProxyHandler(GetSmpProxyFamily())
	{
	}

	bool get(JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver, JS::HandleId id, JS::MutableHandleValue vp) const override;
	bool set(JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v, JS::HandleValue receiver, JS::ObjectOpResult& result) const override;
};

const FbMetadbHandleListProxyHandler FbMetadbHandleListProxyHandler::singleton;

bool FbMetadbHandleListProxyHandler::get(JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver, JS::HandleId id, JS::MutableHandleValue vp) const
{
	if (JSID_IS_INT(id))
	{
		JS::RootedObject target(cx, js::GetProxyTargetObject(proxy));
		auto pNativeTarget = static_cast<JsFbMetadbHandleList*>(JS::GetPrivate(target));
		assert(pNativeTarget);

		const auto index = static_cast<uint32_t>(JSID_TO_INT(id));
		try
		{
			vp.setObjectOrNull(pNativeTarget->get_Item(index));
		}
		catch (...)
		{
			mozjs::error::ExceptionToJsError(cx);
			return false;
		}

		return true;
	}

	return js::ForwardingProxyHandler::get(cx, proxy, receiver, id, vp);
}

bool FbMetadbHandleListProxyHandler::set(JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue v, JS::HandleValue receiver, JS::ObjectOpResult& result) const
{
	if (JSID_IS_INT(id))
	{
		JS::RootedObject target(cx, js::GetProxyTargetObject(proxy));
		auto pNativeTarget = static_cast<JsFbMetadbHandleList*>(JS::GetPrivate(target));
		assert(pNativeTarget);

		const auto index = static_cast<uint32_t>(JSID_TO_INT(id));

		if (!v.isObjectOrNull())
		{
			JS_ReportErrorUTF8(cx, "Value in assignment is of wrong type");
			return false;
		}

		JS::RootedObject jsObject(cx, v.toObjectOrNull());
		JsFbMetadbHandle* pNativeValue =
			jsObject
				? static_cast<JsFbMetadbHandle*>(JS_GetInstancePrivate(cx, jsObject, &JsFbMetadbHandle::JsClass, nullptr))
				: nullptr;

		try
		{
			pNativeTarget->put_Item(index, pNativeValue);
		}
		catch (...)
		{
			mozjs::error::ExceptionToJsError(cx);
			return false;
		}

		result.succeed();
		return true;
	}

	return js::ForwardingProxyHandler::set(cx, proxy, id, v, receiver, result);
}

} // namespace

namespace mozjs
{

const JSClass JsFbMetadbHandleList::JsClass = jsClass;
const JSFunctionSpec* JsFbMetadbHandleList::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsFbMetadbHandleList::JsProperties = jsProperties.data();
const JsPrototypeId JsFbMetadbHandleList::PrototypeId = JsPrototypeId::FbMetadbHandleList;
const JSNative JsFbMetadbHandleList::JsConstructor = ::FbMetadbHandleList_Constructor;
const js::BaseProxyHandler& JsFbMetadbHandleList::JsProxy = FbMetadbHandleListProxyHandler::singleton;

JsFbMetadbHandleList::JsFbMetadbHandleList(JSContext* cx, const metadb_handle_list& handles)
	: pJsCtx_(cx)
	, metadbHandleList_(handles)
{
}

std::unique_ptr<JsFbMetadbHandleList>
JsFbMetadbHandleList::CreateNative(JSContext* cx, const metadb_handle_list& handles)
{
	return std::unique_ptr<JsFbMetadbHandleList>(new JsFbMetadbHandleList(cx, handles));
}

size_t JsFbMetadbHandleList::GetInternalSize(const metadb_handle_list& handles)
{
	return sizeof(metadb_handle) * handles.get_size();
}

const metadb_handle_list& JsFbMetadbHandleList::GetHandleList() const
{
	return metadbHandleList_;
}

JSObject* JsFbMetadbHandleList::Constructor(JSContext* cx, JS::HandleValue jsValue)
{
	if (jsValue.isNullOrUndefined())
	{
		return JsFbMetadbHandleList::CreateJs(cx, metadb_handle_list());
	}

	if (auto pNativeHandle = GetInnerInstancePrivate<JsFbMetadbHandle>(cx, jsValue); pNativeHandle)
	{
		metadb_handle_list handleList;
		handleList.add_item(pNativeHandle->GetHandle());
		return JsFbMetadbHandleList::CreateJs(cx, handleList);
	}

	if (auto pNativeHandleList = GetInnerInstancePrivate<JsFbMetadbHandleList>(cx, jsValue); pNativeHandleList)
	{
		return JsFbMetadbHandleList::CreateJs(cx, pNativeHandleList->GetHandleList());
	}

	{
		metadb_handle_list handleList;
		convert::to_native::ProcessArray<JsFbMetadbHandle*>(cx, jsValue, [&handleList](auto pNativeHandle) {
			qwr::QwrException::ExpectTrue(pNativeHandle, "Array contains invalid value");
			handleList.add_item(pNativeHandle->GetHandle());
		});
		return JsFbMetadbHandleList::CreateJs(cx, handleList);
	}

	throw qwr::QwrException("Unsupported argument type");
}

JSObject* JsFbMetadbHandleList::ConstructorWithOpt(JSContext* cx, size_t optArgCount, JS::HandleValue jsValue)
{
	switch (optArgCount)
	{
	case 0:
		return Constructor(cx, jsValue);
	case 1:
		return Constructor(cx);
	default:
		throw qwr::QwrException("Internal error: invalid number of optional arguments specified: {}", optArgCount);
	}
}

void JsFbMetadbHandleList::Add(JsFbMetadbHandle* handle)
{
	qwr::QwrException::ExpectTrue(handle, "handle argument is null");

	metadb_handle_ptr fbHandle(handle->GetHandle());
	qwr::QwrException::ExpectTrue(fbHandle.is_valid(), "Internal error: FbMetadbHandle does not contain a valid handle");

	metadbHandleList_.add_item(fbHandle);
}

void JsFbMetadbHandleList::AddRange(JsFbMetadbHandleList* handles)
{
	qwr::QwrException::ExpectTrue(handles, "handles argument is null");

	metadbHandleList_.add_items(handles->GetHandleList());
}

void JsFbMetadbHandleList::AttachImage(const std::wstring& image_path, uint32_t art_id)
{
	qwr::QwrException::ExpectTrue(AlbumArtStatic::check_type_id(art_id), "Invalid art_id");
	Attach::from_path(metadbHandleList_, art_id, image_path);
}

int32_t JsFbMetadbHandleList::BSearch(JsFbMetadbHandle* handle)
{
	qwr::QwrException::ExpectTrue(handle, "handle argument is null");

	metadb_handle_ptr fbHandle(handle->GetHandle());
	qwr::QwrException::ExpectTrue(fbHandle.is_valid(), "Internal error: FbMetadbHandle does not contain a valid handle");

	return static_cast<int32_t>(metadbHandleList_.bsearch_by_pointer(fbHandle));
}

double JsFbMetadbHandleList::CalcTotalDuration()
{
	pfc::array_t<double> lengths;
	lengths.set_size(metadbHandleList_.get_count());

	metadb_v2::get()->queryMultiParallel_(metadbHandleList_, [&lengths](size_t index, const metadb_v2::rec_t& rec)
		{
			if (rec.info.is_valid())
			{
				const double length = rec.info->info().get_length();
				lengths[index] = std::max(0.0, length);
			}
		});

	return std::reduce(lengths.begin(), lengths.end());
}

std::uint64_t JsFbMetadbHandleList::CalcTotalSize()
{
	return metadb_handle_list_helper::calc_total_size(metadbHandleList_, true);
}

JSObject* JsFbMetadbHandleList::Clone()
{
	return JsFbMetadbHandleList::CreateJs(pJsCtx_, metadbHandleList_);
}

JS::Value JsFbMetadbHandleList::Convert()
{
	JS::RootedValue jsValue(pJsCtx_);
	convert::to_js::ToArrayValue(pJsCtx_, metadbHandleList_, &jsValue);
	return jsValue;
}

int32_t JsFbMetadbHandleList::Find(JsFbMetadbHandle* handle)
{
	qwr::QwrException::ExpectTrue(handle, "handle argument is null");

	metadb_handle_ptr fbHandle(handle->GetHandle());
	qwr::QwrException::ExpectTrue(fbHandle.is_valid(), "Internal error: FbMetadbHandle does not contain a valid handle");

	return static_cast<int32_t>(metadbHandleList_.find_item(fbHandle));
}

JS::Value JsFbMetadbHandleList::GetLibraryRelativePaths()
{
	const auto count = metadbHandleList_.get_count();
	auto api = library_manager::get();
	pfc::array_t<pfc::string8> values;
	values.set_size(count);

	concurrency::parallel_for(size_t{}, count, [&](size_t index)
		{
			api->get_relative_path(metadbHandleList_[index], values[index]);
		});

	JS::RootedValue jsValue(pJsCtx_);
	convert::to_js::ToArrayValue(pJsCtx_, values, &jsValue);
	return jsValue;
}

void JsFbMetadbHandleList::Insert(uint32_t index, JsFbMetadbHandle* handle)
{
	qwr::QwrException::ExpectTrue(handle, "handle argument is null");

	metadb_handle_ptr fbHandle(handle->GetHandle());
	qwr::QwrException::ExpectTrue(fbHandle.is_valid(), "Internal error: FbMetadbHandle does not contain a valid handle");

	metadbHandleList_.insert_item(fbHandle, index);
}

void JsFbMetadbHandleList::InsertRange(uint32_t index, JsFbMetadbHandleList* handles)
{
	qwr::QwrException::ExpectTrue(handles, "handles argument is null");

	metadbHandleList_.insert_items(handles->GetHandleList(), index);
}

void JsFbMetadbHandleList::MakeDifference(JsFbMetadbHandleList* handles)
{
	qwr::QwrException::ExpectTrue(handles, "handles argument is null");

	metadb_handle_list r1, r2;
	metadb_handle_list_helper::sorted_by_pointer_extract_difference(metadbHandleList_, handles->GetHandleList(), r1, r2);
	metadbHandleList_ = r1;
}

void JsFbMetadbHandleList::MakeIntersection(JsFbMetadbHandleList* handles)
{
	qwr::QwrException::ExpectTrue(handles, "handles argument is null");

	auto& other = handles->GetHandleList();
	metadb_handle_list result;
	size_t walk1{}, walk2{};
	const auto last1 = metadbHandleList_.get_count();
	const auto last2 = other.get_count();

	while (walk1 != last1 && walk2 != last2)
	{
		if (metadbHandleList_[walk1] < other[walk2])
			++walk1;
		else if (other[walk2] < metadbHandleList_[walk1])
			++walk2;
		else
		{
			result.add_item(metadbHandleList_[walk1]);
			++walk1;
			++walk2;
		}
	}

	metadbHandleList_ = result;
}

void JsFbMetadbHandleList::MakeUnion(JsFbMetadbHandleList* handles)
{
	qwr::QwrException::ExpectTrue(handles, "handles argument is null");

	metadbHandleList_.add_items(handles->GetHandleList());
	metadbHandleList_.sort_by_pointer_remove_duplicates();
}

void JsFbMetadbHandleList::OptimiseFileLayout(bool minimise)
{
	if (metadbHandleList_.get_count() == 0)
		return;

	auto func = [handles = metadbHandleList_, minimise](threaded_process_status&, abort_callback& abort)
		{
			auto api = file_lock_manager::get();
			auto paths = file_list_helper::file_list_from_metadb_handle_list(handles);

			for (auto&& path : paths)
			{
				try
				{
					auto lock = api->acquire_write(path, abort);

					for (auto ptr : file_format_sanitizer::enumerate())
					{
						if (ptr->sanitize_file(path, minimise, abort))
							break;
					}
				}
				catch (...) {}
			}
		};

	auto callback = threaded_process_callback_lambda::create(func);
	threaded_process::get()->run_modeless(callback, threaded_process::flag_silent, core_api::get_main_window(), "Optimising...");
}

void JsFbMetadbHandleList::OrderByFormat(JsFbTitleFormat* script, int8_t direction)
{
	qwr::QwrException::ExpectTrue(script, "script argument is null");

	metadbHandleList_.sort_by_format(script->GetTitleFormat(), nullptr, direction);
}

void JsFbMetadbHandleList::OrderByPath()
{
	metadbHandleList_.sort_by_path();
}

void JsFbMetadbHandleList::OrderByRelativePath()
{
	auto api = library_manager::get();

	const auto count = metadbHandleList_.get_count();
	pfc::array_t<CustomSort::Item> items;
	items.set_size(count);

	concurrency::parallel_for(size_t{}, count, [&](size_t index)
		{
			pfc::string8 temp;
			temp.prealloc(512);

			api->get_relative_path(metadbHandleList_[index], temp);
			temp << metadbHandleList_[index]->get_subsong_index();
			items[index].index = index;
			items[index].text = pfc::wideFromUTF8(temp);
		});

	auto order = CustomSort::sort(items);
	metadbHandleList_.reorder(order.get_ptr());
}

void JsFbMetadbHandleList::RefreshStats()
{
	pfc::list_t<metadb_index_hash> hashes;
	for (const auto& handle: metadbHandleList_)
	{
		metadb_index_hash hash;
		if (stats::HashHandle(handle, hash))
		{
			hashes.add_item(hash);
		}
	}

	stats::RefreshStats(hashes);
}

void JsFbMetadbHandleList::Remove(JsFbMetadbHandle* handle)
{
	qwr::QwrException::ExpectTrue(handle, "handle argument is null");

	metadb_handle_ptr fbHandle(handle->GetHandle());
	qwr::QwrException::ExpectTrue(fbHandle.is_valid(), "Internal error: FbMetadbHandle does not contain a valid handle");

	metadbHandleList_.remove_item(fbHandle);
}

void JsFbMetadbHandleList::RemoveAll()
{
	metadbHandleList_.remove_all();
}

void JsFbMetadbHandleList::RemoveAttachedImage(uint32_t art_id)
{
	qwr::QwrException::ExpectTrue(AlbumArtStatic::check_type_id(art_id), "Invalid art_id");
	Attach::remove_id(metadbHandleList_, art_id);
}

void JsFbMetadbHandleList::RemoveAttachedImages()
{
	Attach::remove_all(metadbHandleList_);
}

void JsFbMetadbHandleList::RemoveById(uint32_t index)
{
	qwr::QwrException::ExpectTrue(index < metadbHandleList_.get_count(), "Index is out of bounds");
	(void)metadbHandleList_.remove_by_idx(index);
}

void JsFbMetadbHandleList::RemoveRange(uint32_t from, uint32_t count)
{
	metadbHandleList_.remove_from_idx(from, count);
}

void JsFbMetadbHandleList::SaveAs(const std::string& path)
{
	try
	{
		playlist_loader::g_save_playlist(path.c_str(), metadbHandleList_, fb2k::noAbort);
	}
	catch (...) {}
}

void JsFbMetadbHandleList::Sort()
{
	metadbHandleList_.sort_by_pointer_remove_duplicates();
}

void JsFbMetadbHandleList::UpdateFileInfoFromJSON(const std::string& str)
{
	const auto count = metadbHandleList_.get_count();

	if (count == 0)
	{
		return;
	}

	auto j = JSON::parse(str, nullptr, false);
	auto writer = TagWriter(metadbHandleList_);

	if (j.is_array())
	{
		qwr::QwrException::ExpectTrue(j.size() == count, "Invalid JSON info: mismatched with handle count");
		writer.from_json_array(j);
	}
	else if (j.is_object())
	{
		qwr::QwrException::ExpectTrue(j.size() > 0, "Invalid JSON info: empty object");
		writer.from_json_object(j);
	}
	else
	{
		throw qwr::QwrException("Invalid JSON info: unsupported value type");
	}
}

JSObject* JsFbMetadbHandleList::CreateIterator()
{
	return JsFbMetadbHandleList_Iterator::CreateJs(pJsCtx_, *this);
}

uint32_t JsFbMetadbHandleList::get_Count()
{
	return metadbHandleList_.get_count();
}

JSObject* JsFbMetadbHandleList::get_Item(uint32_t index)
{
	qwr::QwrException::ExpectTrue(index < metadbHandleList_.get_count(), "Index is out of bounds");

	return JsFbMetadbHandle::CreateJs(pJsCtx_, metadbHandleList_[index]);
}

void JsFbMetadbHandleList::put_Item(uint32_t index, JsFbMetadbHandle* handle)
{
	qwr::QwrException::ExpectTrue(index < metadbHandleList_.get_count(), "Index is out of bounds");
	qwr::QwrException::ExpectTrue(handle, "handle argument is null");

	metadb_handle_ptr fbHandle(handle->GetHandle());
	qwr::QwrException::ExpectTrue(fbHandle.is_valid(), "Internal error: FbMetadbHandle does not contain a valid handle");

	metadbHandleList_.replace_item(index, fbHandle);
}

} // namespace mozjs
