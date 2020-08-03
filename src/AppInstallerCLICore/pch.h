// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <WinInet.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.ApplicationModel.Resources.h>
#include <winrt/Windows.ApplicationModel.Resources.Core.h>
#include <winrt/Windows.ApplicationModel.Store.Preview.InstallControl.h>

#include <wil/result_macros.h>
#include <wil/safecast.h>

#include <array>
#include <iostream>
#include <fstream>
#include <future>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>

#include <yaml-cpp\yaml.h>

#include <wrl/client.h>
#include <AppxPackaging.h>

#include <AppInstallerDateTime.h>
#include <AppInstallerDeployment.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerMsixInfo.h>
#include <AppInstallerRepositorySearch.h>
#include <AppInstallerRepositorySource.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerSHA256.h>
#include <AppInstallerStrings.h>
#include <AppInstallerTelemetry.h>
#include <Manifest/YamlParser.h>
#include <winget/LocIndependent.h>
#include <winget/ExperimentalFeature.h>
