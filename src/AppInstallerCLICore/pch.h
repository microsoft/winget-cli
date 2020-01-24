// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <windows.h>
#include <WinInet.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>

#include <wil/result_macros.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>
#include <future>
#include <string_view>

#include <yaml-cpp\yaml.h>

#include "AppInstallerLogging.h"
#include "AppInstallerTelemetry.h"
#include "AppInstallerStrings.h"
#include "AppInstallerRuntime.h"
#include "AppInstallerSHA256.h"
#include "AppInstallerDownloader.h"
#include "AppInstallerErrors.h"
#include "Manifest/ManifestInstaller.h"
#include "Manifest/Manifest.h"
