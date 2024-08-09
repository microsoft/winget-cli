// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "MockConnectionManager.h"

#include "MockConnection.h"

using namespace SFS::details;

MockConnectionManager::MockConnectionManager(const ReportingHandler& handler) : ConnectionManager(handler)
{
}

MockConnectionManager::~MockConnectionManager()
{
}

std::unique_ptr<Connection> MockConnectionManager::MakeConnection(const ConnectionConfig& config)
{
    return std::make_unique<MockConnection>(config, m_handler);
}
