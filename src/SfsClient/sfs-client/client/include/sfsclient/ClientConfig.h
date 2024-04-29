// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Logging.h"

#include <optional>
#include <string>

namespace SFS
{
/// @brief Configurations to create an SFSClient instance
struct ClientConfig
{
    /// @brief The account ID of the SFS service is used to identify the caller (required)
    std::string accountId;

    /// @brief The instance ID of the SFS service
    std::optional<std::string> instanceId;

    /// @brief The namespace of the SFS service
    std::optional<std::string> nameSpace;

    /**
     * @brief A logging callback function that is called when the SFSClient logs a message
     * @details This function returns logging information from the SFSClient. The caller is responsible for incoporating
     * the received data into their logging system. The callback will be called in the same thread as the
     * main flow, so make sure the callback does not block for too long so it doesn't delay operations. The
     * LogData does not exist after the callback returns, so caller has to copy it if the data will be stored.
     */
    std::optional<LoggingCallbackFn> logCallbackFn;
};
} // namespace SFS
