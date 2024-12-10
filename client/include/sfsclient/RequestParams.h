// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace SFS
{
using TargetingAttributes = std::unordered_map<std::string, std::string>;

constexpr unsigned c_maxRetries = 3;

struct ProductRequest
{
    /// @brief The name or GUID that uniquely represents the product in the service (required)
    std::string product;

    /// @brief Key-value pair to filter the data retrieved from the service. Known from publishing (optional)
    TargetingAttributes attributes;
};

/// @brief Configurations to perform a request to the SFS service
struct RequestParams
{
    /// @brief List of products to be retrieved from the server (required)
    /// @note At the moment only a single product request is supported. Using a vector for future implementation of
    /// batch requests
    std::vector<ProductRequest> productRequests;

    /// @brief Base CorrelationVector to be used in the request for service telemetry stitching (optional)
    /// @note If not provided, a new CorrelationVector will be generated
    std::optional<std::string> baseCV;

    /// @brief Proxy setting which can be used to establish connections with the server (optional)
    /// @note The string can be a hostname or dotted numerical IP address. It can be suffixed with the port number
    /// like :[port], and can be prefixed with [scheme]://. If not provided, no proxy will be used.
    std::optional<std::string> proxy;

    /// @brief Retry for a web request after a failed attempt. If true, client will retry up to c_maxRetries times
    bool retryOnError{true};
};
} // namespace SFS
