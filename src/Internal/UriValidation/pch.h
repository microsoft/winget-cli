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

#include <winrt/Windows.ApplicationModel.Resources.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.System.Profile.h>
