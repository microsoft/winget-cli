// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "../Connection.h"

#include <string>

namespace SFS::details
{
class ReportingHandler;

class MockConnection : public Connection
{
  public:
    MockConnection(const ConnectionConfig& config, const ReportingHandler& handler);
    ~MockConnection() override;

    std::string Get(const std::string& url) override;
    std::string Post(const std::string& url, const std::string& data) override;
};
} // namespace SFS::details
