// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX
#include <windows.h>
#include <urlmon.h>

#include <wil/resource.h>
#include <wil/result_macros.h>

#include <AppInstallerDateTime.h>
#include <AppInstallerDeployment.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerMsixInfo.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerSHA256.h>
#include <AppInstallerStrings.h>
#include <AppInstallerSynchronization.h>
#include <AppInstallerTelemetry.h>
#include <AppInstallerVersions.h>
#include <winget/ExtensionCatalog.h>
#include <winget/ExperimentalFeature.h>
#include <winget/Settings.h>
#include <winget/UserSettings.h>
#include <winget/Yaml.h>

#include <winsqlite/winsqlite3.h>

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.Storage.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <sstream>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
