// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <WinInet.h>
#include <msi.h>
#include <wincodec.h>

#pragma warning( push )
#pragma warning ( disable : 4458 4100 6031 4702 )
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>
#include <valijson/adapters/jsoncpp_adapter.hpp>
#pragma warning( pop )

#include <algorithm>
#include <array>
#include <atomic>
#include <csignal>
#include <iostream>
#include <fstream>
#include <future>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Management.Deployment.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Windows.ApplicationModel.Resources.h>
#include <winrt/Windows.ApplicationModel.Resources.Core.h>
#include <winrt/Windows.ApplicationModel.Store.Preview.InstallControl.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Security.Cryptography.Certificates.h>

#pragma warning( push )
#pragma warning ( disable : 6001 6285 6340 6388 )
#include <wil/result.h>
#include <wil/result_macros.h>
#include <wil/safecast.h>
#include <wil/com.h>
#pragma warning( pop )

#include <wrl/client.h>
#include <AppxPackaging.h>
