// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX
#include <Windows.h>
#include <AclAPI.h>
#include <appmodel.h>
#include <sddl.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <wow64apiset.h>
#include <icu.h>
#include <msi.h>
#include <DbgHelp.h>
#include <SoftPub.h>
#include <WinTrust.h>
#include <wincrypt.h>
#include <deliveryoptimization.h>
#include <deliveryoptimizationerrors.h>

#include "TraceLogging.h"

#define YAML_DECLARE_STATIC
#include <yaml.h>

// TODO: See if we can get down to having just one JSON parser...
#include <json/json.h>

#pragma warning( push )
#pragma warning ( disable : 4458 4100 4702 6031 )
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>
#include <valijson/adapters/jsoncpp_adapter.hpp>
#pragma warning( pop )

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <regex>
#include <set>
#include <string>
#include <sstream>
#include <stack>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>
#include <variant>

#ifndef WINGET_DISABLE_FOR_FUZZING
#pragma warning( push )
#pragma warning ( disable : 26495 26439 )
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <cpprest/uri_builder.h>
#pragma warning( pop )
#endif

#pragma warning( push )
#pragma warning ( disable : 6001 6285 6287 6340 6387 6388 26451 26495 28196 )
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/result_macros.h>
#include <wil/safecast.h>
#include <wil/token_helpers.h>
#include <wil/com.h>
#include <wil/filesystem.h>
#include <wil/win32_helpers.h>
#pragma warning( pop )

#include <wil/cppwinrt.h>

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.AppExtensions.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Services.Store.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.Profile.h>
#include <winrt/Windows.System.UserProfile.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Headers.h>
#include <winrt/Windows.Web.Http.Filters.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.ApplicationModel.Store.Preview.InstallControl.h>
#include <winrt/Windows.Security.Authentication.Web.Core.h>
#include <winrt/Windows.Security.Credentials.h>

#include <wrl/client.h>
#include <wrl/implements.h>

// Stream/buffer helper APIs
#include <robuffer.h>
#include <shcore.h>

#include <AppxPackaging.h>
#include <WebAuthenticationCoreManagerInterop.h>
