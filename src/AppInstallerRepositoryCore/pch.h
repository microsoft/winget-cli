// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX
#include <windows.h>
#include <urlmon.h>
#include <appmodel.h>
#include <Shlwapi.h>
#include <Shlobj.h>
#include <msi.h>

#pragma warning( push )
#pragma warning ( disable : 6001 6340 6387 6388 26495 28196 )
#include <wil/filesystem.h>
#include <wil/registry.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/result_macros.h>
#pragma warning( pop )

#include <winsqlite/winsqlite3.h>

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Web.Http.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <sstream>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <json/json.h>

#pragma warning( push )
#pragma warning ( disable : 26495 26439 )
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <cpprest/uri_builder.h>
#pragma warning( pop )

#include <AppInstallerDateTime.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerSHA256.h>
#include <AppInstallerStrings.h>
#include <AppInstallerSynchronization.h>
#include <AppInstallerTelemetry.h>
#include <AppInstallerVersions.h>
#include <winget/ExtensionCatalog.h>
#include <winget/ExperimentalFeature.h>
#include <winget/Locale.h>
#include <winget/Settings.h>
#include <winget/UserSettings.h>
#include <winget/Yaml.h>
