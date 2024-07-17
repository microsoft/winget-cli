// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>

#pragma warning( push )
#pragma warning( disable : 4324 4467 6388 6553 )
// 4324 Allow use of alignment specifiers
// 4467 Allow use of uuid attribute for com object creation.
// 6388 Allow CreateInstance.
// 6553 Allow annotation to value type
#include <wil/cppwinrt_wrl.h>
#include <wil/resource.h>
#pragma warning( pop )

#include <atomic>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
