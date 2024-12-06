// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Result.h"

#include <memory>

namespace SFS::test
{
namespace details
{
class ProxyServerImpl;
}

// Proxy Server implementation that redirects GET and POST requests directly
class ProxyServer
{
  public:
    ProxyServer();
    ~ProxyServer();

    ProxyServer(const ProxyServer&) = delete;
    ProxyServer& operator=(const ProxyServer&) = delete;

    Result Stop();

    std::string GetBaseUrl() const;

  private:
    std::unique_ptr<details::ProxyServerImpl> m_impl;
};
} // namespace SFS::test
