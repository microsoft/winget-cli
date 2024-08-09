// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX

#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>

#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 4467 Allow use of uuid attribute for com object creation.
// 6388 Allow CreateInstance.
#include <wil/cppwinrt_wrl.h>
#include <wil/resource.h>
#pragma warning( pop )

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
