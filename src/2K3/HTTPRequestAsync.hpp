#pragma once
#include <cpr/cpr.h>

class HTTPRequestAsync : public fb2k::threadEntry
{
public:
	enum class Type
	{
		GET,
		POST,
	};

	HTTPRequestAsync(Type type, HWND wnd, uint32_t task_id, std::string_view url, std::string_view user_agent_or_headers, std::string_view body);

	void run() noexcept final;

private:
	static bool is_supported_content_type(std::string_view content_type) noexcept;

	cpr::Response get_response() noexcept;
	void add_headers() noexcept;

	HWND m_wnd;
	Type m_type{};
	cpr::Body m_body;
	cpr::Session m_session;
	cpr::Url m_url;
	std::string m_user_agent_or_headers;
	uint32_t m_task_id{};
};
