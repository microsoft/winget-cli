// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "ConnectionManager.h"

#include <memory>

namespace SFS::details
{
class Connection;
class ReportingHandler;
struct ConnectionConfig;

class CurlConnectionManager : public ConnectionManager
{
  public:
    CurlConnectionManager(const ReportingHandler& handler);
    ~CurlConnectionManager() override;

    std::unique_ptr<Connection> MakeConnection(const ConnectionConfig& config) override;
};
} // namespace SFS::details
