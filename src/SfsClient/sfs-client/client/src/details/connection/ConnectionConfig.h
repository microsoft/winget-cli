// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <optional>
#include <string>

namespace SFS
{
struct RequestParams;

namespace details
{
struct ConnectionConfig
{
    ConnectionConfig() = default;
    explicit ConnectionConfig(const RequestParams& requestParams);

    /// @brief Expected number of retries for a web request after a failed attempt
    unsigned maxRetries{3};

    /// @brief The correlation vector to use for requests
    std::optional<std::string> baseCV;
};
} // namespace details
} // namespace SFS
