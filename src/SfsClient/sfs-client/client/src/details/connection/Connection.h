// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "../CorrelationVector.h"
#include "ConnectionConfig.h"

#include <string>

namespace SFS::details
{
class ReportingHandler;

class Connection
{
  public:
    Connection(const ConnectionConfig& config, const ReportingHandler& handler);

    virtual ~Connection()
    {
    }

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    /**
     * @brief Perform a GET request to the given @param url
     * @return The response body
     * @throws SFSException if the request fails
     */
    virtual std::string Get(const std::string& url) = 0;

    /**
     * @brief Perform a POST request to the given @param url with @param data as the request body
     * @return The response body
     * @throws SFSException if the request fails
     */
    virtual std::string Post(const std::string& url, const std::string& data) = 0;

    /**
     * @brief Perform a POST request to the given @param url
     * @return The response body
     * @throws SFSException if the request fails
     */
    std::string Post(const std::string& url);

  protected:
    const ReportingHandler& m_handler;

    /// @brief The correlation vector to use for requests
    CorrelationVector m_cv;

    /// @brief Expected number of retries for a web request after a failed attempt
    unsigned m_maxRetries{3};
};
} // namespace SFS::details
