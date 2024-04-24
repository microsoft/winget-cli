// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "sfsclient/Logging.h"

#include <chrono>

#define TEST_UNSCOPED_INFO(message)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        const std::string __message = (message);                                                                       \
        SFS::test::LogCallbackToTest({SFS::LogSeverity::Info,                                                          \
                                      __message.c_str(),                                                               \
                                      __FILE__,                                                                        \
                                      __LINE__,                                                                        \
                                      __FUNCTION__,                                                                    \
                                      std::chrono::system_clock::now()});                                              \
    } while ((void)0, 0)

namespace SFS::test
{
// Use this method to redirect the library logging to the Catch2 logging system
void LogCallbackToTest(const SFS::LogData& logData);
} // namespace SFS::test
