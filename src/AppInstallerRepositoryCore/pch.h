// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX
#include <windows.h>
#include <urlmon.h>

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
#include <AppInstallerVersions.h>
#include <winget/ExtensionCatalog.h>
#include <winget/Settings.h>
#include <yaml-cpp/yaml.h>

#include <wil/result_macros.h>

#include <winsqlite/winsqlite3.h>

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iomanip>
#include <optional>
#include <string>
#include <string_view>
#include <sstream>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <functional>
#include <regex>

#ifndef AICLI_DISABLE_TEST_HOOKS
#include <functional>
#include <map>
#endif
