// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Result.h"

#include <memory>
#include <queue>
#include <string>
#include <unordered_map>

namespace SFS::details
{
enum class HttpHeader;
}

namespace SFS::test
{
namespace details
{
class MockWebServerImpl;
}

using HttpCode = int;
using HeaderMap = std::unordered_map<std::string, std::string>;

struct MockPrerequisite
{
    std::string name;
    std::string version;
};

class MockWebServer
{
  public:
    MockWebServer();
    ~MockWebServer();

    MockWebServer(const MockWebServer&) = delete;
    MockWebServer& operator=(const MockWebServer&) = delete;

    [[nodiscard]] Result Stop();

    std::string GetBaseUrl() const;

    /// @brief Registers a product with the server. Will fill the other data with gibberish for testing purposes
    void RegisterProduct(std::string name, std::string version);

    /// @brief Registers an app with the server. Will fill the other data with gibberish for testing purposes
    void RegisterAppProduct(std::string name, std::string version, std::vector<MockPrerequisite> prerequisites);

    /// @brief Registers the expectation of a given header to the present in the request
    void RegisterExpectedRequestHeader(SFS::details::HttpHeader header, std::string value);

    /**
     * @brief Registers a sequence of HTTP error codes that will be sent by the server in the order in which they are
     * passed.
     */
    void SetForcedHttpErrors(std::queue<HttpCode> forcedErrors);

    /// @brief Registers a set of headers that will be sent depending on the HTTP code
    void SetResponseHeaders(std::unordered_map<HttpCode, HeaderMap> headersByCode);

  private:
    std::unique_ptr<details::MockWebServerImpl> m_impl;
};
} // namespace SFS::test
