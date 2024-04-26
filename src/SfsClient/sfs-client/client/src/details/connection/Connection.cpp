// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Connection.h"

using namespace SFS;
using namespace SFS::details;

Connection::Connection(const ConnectionConfig& config, const ReportingHandler& handler) : m_handler(handler)
{
    if (config.baseCV)
    {
        m_cv = std::move(CorrelationVector(*config.baseCV, m_handler));
    }
    m_maxRetries = config.maxRetries;
}

std::string Connection::Post(const std::string& url)
{
    return Post(url, {});
}
