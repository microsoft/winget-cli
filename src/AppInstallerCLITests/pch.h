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

#pragma warning( push )
#pragma warning ( disable : 26439 26451 26495 )
#include <catch.hpp>
#pragma warning( pop )

#include <json/json.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Management.Deployment.h>

#pragma warning( push )
#pragma warning ( disable : 6001 6553 6387 6388 26495 28193 28196 )
#include <wil/filesystem.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/result_macros.h>
#include <wil/token_helpers.h>
#pragma warning( push )

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

// The compiler complains about MarketsInfo::ExcludedMarkets being uninitialized,
// only in this project for some reason.
// Adding = {} didn't fix it, so disabling warning here.
#pragma warning( push )
#pragma warning ( disable : 26495 )
#include <winget/ManifestInstaller.h>
#pragma warning( pop )