// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <string_view>

namespace SFS::details::util
{
bool AreEqualI(std::string_view a, std::string_view b);
bool AreNotEqualI(std::string_view a, std::string_view b);
} // namespace SFS::details::util
