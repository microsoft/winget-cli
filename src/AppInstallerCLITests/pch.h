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

#include <catch2/catch_session.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

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

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>
