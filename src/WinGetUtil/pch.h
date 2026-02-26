// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#define NOMINMAX
#include <Windows.h>

#pragma warning( push )
#pragma warning ( disable : 6001 6340 6387 6388 26495 28196 )
#include <wil/result.h>
#include <wil/result_macros.h>
#include <wil/filesystem.h>
#pragma warning( pop )

#include <algorithm>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Web.Http.h>
