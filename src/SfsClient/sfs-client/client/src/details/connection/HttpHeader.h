// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <string>

namespace SFS::details
{
enum class HttpHeader
{
    ContentType,
    MSCV,
    RetryAfter,
    UserAgent,
};

std::string ToString(HttpHeader header);

std::string GetUserAgentValue();
} // namespace SFS::details
