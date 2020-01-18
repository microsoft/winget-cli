// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX
#include <Windows.h>
#include <appmodel.h>
#include <WinInet.h>

#include "TraceLogging.h"

#include <wil/result_macros.h>
#include <wil/safecast.h>
#include <wil/resource.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <future>