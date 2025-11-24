#include <stdafx.h>
#include "DownloadFileAsync.hpp"
#include "FileHelper.hpp"

#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>

DownloadFileAsync::DownloadFileAsync(HWND wnd, std::string_view url, std::wstring_view path)
	: m_wnd(wnd)
	, m_url(url)
	, m_path(path) {}

void DownloadFileAsync::run() noexcept
{
	bool success{};
	std::string error_text;

	try
	{
		auto& aborter = fb2k::mainAborter();

		auto request = http_client::get()->create_request("GET");
		request->add_header("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:144.0) Gecko/20100101 Firefox/144.0");

		auto response = request->run(m_url.c_str(), aborter);

		// throws exception if size is unknown
		std::ignore = response->get_size_ex(aborter);

		pfc::array_t<uint8_t> arr;
		response->read_till_eof(arr, aborter);

		success = FileHelper(m_path).write(arr.get_ptr(), arr.get_size());
		QwrException::ExpectTrue(success, L"Error saving downloaded file to: {}", m_path);
	}
	catch (const std::exception& e)
	{
		error_text = e.what();
	}

	smp::EventDispatcher::Get().PutEvent(
		m_wnd,
		smp::GenerateEvent_JsCallback(
			smp::EventId::kInternalDownloadFileDone,
			m_path,
			success,
			error_text
		)
	);
}
