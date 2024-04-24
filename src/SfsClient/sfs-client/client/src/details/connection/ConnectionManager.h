// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <memory>

namespace SFS::details
{
class Connection;
class ReportingHandler;
struct ConnectionConfig;

class ConnectionManager
{
  public:
    ConnectionManager(const ReportingHandler& handler);
    virtual ~ConnectionManager();

    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    virtual std::unique_ptr<Connection> MakeConnection(const ConnectionConfig& config) = 0;

  protected:
    const ReportingHandler& m_handler;
};
} // namespace SFS::details
