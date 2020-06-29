// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX
#include <Windows.h>
#include <appmodel.h>
#include <WinInet.h>
#include <sddl.h>
#include <Shlobj.h>
#include <Shlwapi.h>

#include "TraceLogging.h"

#include <yaml-cpp/yaml.h>

// wil/cppwinrt.h should always be included before any C++/WinRT or WIL header file when both are in use
#include <wil/cppwinrt.h>
#include <wil/result_macros.h>
#include <wil/safecast.h>
#include <wil/resource.h>
#include <wil/token_helpers.h>

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.AppExtensions.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.Profile.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Headers.h>
#include <winrt/Windows.Web.Http.Filters.h>

#include <wrl/client.h>

// Stream/buffer helper APIs
#include <robuffer.h>
#include <shcore.h>

#include <AppxPackaging.h>

#include <chrono>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <vector>
