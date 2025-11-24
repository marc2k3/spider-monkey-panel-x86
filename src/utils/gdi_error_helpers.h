#pragma once
#include <utils/gdi_helpers.h>

namespace qwr
{
	[[nodiscard]] std::string GdiErrorCodeToText(Gdiplus::Status errorCode);

	/// @throw QwrException
	void CheckGdi(Gdiplus::Status gdiStatus, std::string_view functionName);

	/// @throw QwrException
	template <typename T, typename T_Parent = T>
	void CheckGdiPlusObject(const std::unique_ptr<T>& obj, const T_Parent* pParentObj = nullptr)
	{
		// GetLastStatus() resets status, so it needs to be saved here
		const auto status = [&obj, pParentObj]() -> std::optional<Gdiplus::Status>
			{
				if (obj)
					return obj->GetLastStatus();
				else if (pParentObj)
					return pParentObj->GetLastStatus();
				else
					return std::nullopt;
			}();

		if (obj && Gdiplus::Status::Ok == status)
			return;

		if (status)
		{
			throw QwrException("Failed to create GdiPlus object ({:#x}): {}", static_cast<int>(*status), GdiErrorCodeToText(*status));
		}
		else
		{
			throw QwrException("Failed to create GdiPlus object");
		}
	}
}
