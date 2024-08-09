// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#define NOMINMAX
#include <Windows.h>
#include <AclAPI.h>
#include <WinInet.h>
#include <shellapi.h>
#include <objbase.h>
#include <urlmon.h>
#include <Msi.h>
#include <KnownFolders.h>

#include <catch.hpp>

#include <json/json.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.Web.Http.h>
#include <wrl/client.h>

#include <wil/filesystem.h>
#include <wil/resource.h>
#include <wil/result_macros.h>
#include <wil/token_helpers.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>
