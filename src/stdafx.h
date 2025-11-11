#pragma once
#define _ATL_MODULES
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WINVER _WIN32_WINNT_WIN7
#define NOMINMAX

// stl
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include <WinSock2.h>
#include <Windows.h>
#include <windowsx.h>
#include <GdiPlus.h>
#include <Shlwapi.h>
#include <wincodec.h>
#include <windef.h>

// COM objects
#include <ActivScp.h>
#include <activdbg.h>
#include <comutil.h>
#include <MsHTML.h>
#include <MsHtmHst.h>
#include <ShlDisp.h>
#include <ShlObj.h>
#include <exdisp.h>
#include <shobjidl_core.h>
// Generates wrappers for COM listed above
#include <ComDef.h>

#include <atlstr.h>
#include <atlapp.h>
#include <atlcom.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlddx.h>
#include <atldlgs.h>
#include <atlfind.h>
#include <atlframe.h>
#include <atltypes.h>
#include <atlwin.h>

// 4251: dll interface warning
#define SMP_MJS_SUPPRESS_WARNINGS_PUSH \
	__pragma(warning(push))        \
	__pragma(warning(disable : 4251))

#define SMP_MJS_SUPPRESS_WARNINGS_POP \
	__pragma(warning(pop))

// Mozilla SpiderMonkey
SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <jsapi.h>
#include <jsfriendapi.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

// fmt
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <fmt/xchar.h>

// range v3
#include <range/v3/all.hpp>

// json
#define JSON_DIAGNOSTICS 1
#define JSON_USE_IMPLICIT_CONVERSIONS 0
#define JSON_HAS_FILESYSTEM 0
#include <nlohmann/json.hpp>
using JSON = nlohmann::json;

// wil
#include <wil/com.h>
#include <wil/filesystem.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>

// foobar2000 SDK
#pragma warning(push, 0)
#include <foobar2000/SDK/foobar2000.h>
#include <foobar2000/helpers/file_list_helper.h>
#include <libPPUI/pp-COM-macros.h>
#include <pfc/string-conv-lite.h>
#pragma warning(pop)

// Columns UI SDK
#pragma warning(push, 0)
#include <columns_ui-sdk/ui_extension.h>
#pragma warning(pop)

// fb2k_utils
#include <qwr/unicode.h>
#include <qwr/qwr_exception.h>

#include <utils/js_exception.h>
#include <component_defines.h>
#include <component_guids.h>
#include <component_paths.h>
#include <2K3/String.hpp>
