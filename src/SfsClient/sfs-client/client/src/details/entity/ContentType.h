// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <string>

namespace SFS::details
{
enum class ContentType
{
    Generic,
    App,
};

std::string ToString(ContentType type);
} // namespace SFS::details
