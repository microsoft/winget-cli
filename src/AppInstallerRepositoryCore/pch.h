// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX
#include <windows.h>

#include "yaml-cpp/yaml.h"

#include "TelemetryWrapper.h"

#include <wil/result_macros.h>

#include <winsqlite/winsqlite3.h>

#include <winrt/Windows.Foundation.h>

#include <string>
#include <stdexcept>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
