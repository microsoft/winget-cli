// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX
#include <Windows.h>
#include <AclAPI.h>
#include <appmodel.h>
#include <icu.h>
#include <sddl.h>
#include <Shlobj.h>
#include <compressapi.h>
#include <concurrencysal.h>

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
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <vector>

#pragma warning( push )
#pragma warning ( disable : 6001 6285 6287 6340 6387 6388 26495 28196 )
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/result_macros.h>
#include <wil/safecast.h>
#include <wil/token_helpers.h>
#include <wil/com.h>
#include <wil/filesystem.h>
#pragma warning( pop )

#include <wil/cppwinrt.h>
#include <winrt/Windows.ApplicationModel.Resources.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.System.Profile.h>
