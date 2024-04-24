// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "TestHelper.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <time.h>

#include <catch2/catch_test_macros.hpp>

static std::mutex s_logMutex;

namespace
{
std::string TimestampToString(std::chrono::time_point<std::chrono::system_clock> time)
{
    using namespace std::chrono;

    // get number of milliseconds for the current second
    // (remainder after division into seconds)
    auto ms = duration_cast<milliseconds>(time.time_since_epoch()) % 1000;

    auto timer = system_clock::to_time_t(time);

    std::stringstream timeStream;
    struct tm gmTime;
#ifdef _WIN32
    gmtime_s(&gmTime, &timer); // gmtime_s is the safe version of gmtime, not available on Linux
#else
    gmTime = (*std::gmtime(&timer));
#endif
    timeStream << std::put_time(&gmTime, "%F %X"); // yyyy-mm-dd HH:MM:SS
    timeStream << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return timeStream.str();
}
} // namespace

void SFS::test::LogCallbackToTest(const SFS::LogData& logData)
{
    std::lock_guard guard(s_logMutex);
    UNSCOPED_INFO("Log: " << TimestampToString(logData.time) << " [" << ToString(logData.severity) << "]" << " "
                          << std::filesystem::path(logData.file).filename().string() << ":" << logData.line << " "
                          << logData.message);
}
