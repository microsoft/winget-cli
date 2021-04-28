// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX
#include <Windows.h>

#pragma warning( push )
#pragma warning ( disable : 6001 6340 6388 )
#include <wil/result.h>
#include <wil/result_macros.h>
#pragma warning( pop )

#include <algorithm>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
