// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#define NOMINMAX
#include <Windows.h>
#include <WinInet.h>

#include <catch.hpp>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Management.Deployment.h>

#include <wil/result_macros.h>

#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>