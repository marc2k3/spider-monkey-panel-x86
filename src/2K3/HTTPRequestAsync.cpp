#include <stdafx.h>
#include "HTTPRequestAsync.hpp"

#include <events/event_dispatcher.h>
#include <events/event_js_callback.h>

HTTPRequestAsync::HTTPRequestAsync(Type type, HWND wnd, uint32_t task_id, std::string_view url, std::string_view user_agent_or_headers, std::string_view body)
	: m_type(type)
	, m_wnd(wnd)
	, m_task_id(task_id)
	, m_url(url)
	, m_user_agent_or_headers(user_agent_or_headers)
	, m_body(body) {}

#pragma region static
bool HTTPRequestAsync::is_supported_content_type(std::string_view content_type) noexcept
{
	return content_type.contains("text") || content_type.contains("json") || content_type.contains("xml");
}
#pragma endregion

cpr::Response HTTPRequestAsync::get_response() noexcept
{
	m_session.SetUrl(m_url);
	add_headers();

	if (m_type == Type::GET)
		return m_session.Get();

	m_session.SetBody(m_body);
	return m_session.Post();
}

void HTTPRequestAsync::add_headers() noexcept
{
	if (m_user_agent_or_headers.empty())
	{
		m_session.SetUserAgent(SMP_USER_AGENT);
	}
	else
	{
		auto j = JSON::parse(m_user_agent_or_headers, nullptr, false);

		if (j.is_object())
		{
			cpr::Header header;

			for (const auto& [name, value] : j.items())
			{
				if (name.empty())
					continue;

				const auto value_string = json_to_string(value);

				if (value_string.empty())
					continue;

				header.emplace(name, value_string);
			}

			m_session.SetHeader(header);
		}
		else
		{
			m_session.SetUserAgent(m_user_agent_or_headers);
		}
	}
}

void HTTPRequestAsync::run() noexcept
{
	bool success{};
	std::string response_text;

	auto response_headers = JSON::object();
	auto r = get_response();
	const auto status = static_cast<int>(r.status_code);

	if (status == 0)
	{
		response_text = r.error.message;
	}
	else
	{
		auto& content_type = r.header["Content-Type"];
		success = is_supported_content_type(content_type);

		if (success)
		{
			response_text = r.text;
			response_headers = JSON(r.header);
		}
		else
		{
			response_text = fmt::format("Unsupported content type: {}", content_type);
		}
	}

	smp::EventDispatcher::Get().PutEvent(
		m_wnd,
		smp::GenerateEvent_JsCallback(
			smp::EventId::kInternalHttpRequestDone,
			m_task_id,
			success,
			response_text,
			status,
			response_headers.dump(4)
		)
	);
}
