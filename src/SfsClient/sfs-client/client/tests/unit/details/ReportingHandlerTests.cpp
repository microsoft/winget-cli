// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ReportingHandler.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <mutex>
#include <optional>
#include <thread>

#define TEST(...) TEST_CASE("[ReportingHandlerTests] " __VA_ARGS__)

using namespace SFS;
using namespace SFS::details;

TEST("Testing SetLoggingCallback()")
{
    ReportingHandler handler;

    bool called = false;
    auto handling = [&](const LogData&) { called = true; };

    handler.SetLoggingCallback(handling);
    REQUIRE_FALSE(called);

    LOG_INFO(handler, "Test");

    called = false;
    handler.SetLoggingCallback(nullptr);

    LOG_INFO(handler, "Test");
    REQUIRE_FALSE(called);
}

TEST("Testing Severities")
{
    ReportingHandler handler;

    std::optional<LogSeverity> severity;
    auto handling = [&](const LogData& data) { severity = data.severity; };

    handler.SetLoggingCallback(handling);

    REQUIRE(!severity.has_value());

    LOG_INFO(handler, "Test");
    REQUIRE(severity.has_value());
    REQUIRE(*severity == LogSeverity::Info);
    severity.reset();

    LOG_WARNING(handler, "Test");
    REQUIRE(severity.has_value());
    REQUIRE(*severity == LogSeverity::Warning);
    severity.reset();

    LOG_ERROR(handler, "Test");
    REQUIRE(severity.has_value());
    REQUIRE(*severity == LogSeverity::Error);
    severity.reset();

    LOG_VERBOSE(handler, "Test");
    REQUIRE(severity.has_value());
    REQUIRE(*severity == LogSeverity::Verbose);

    handler.SetLoggingCallback(nullptr);
}

TEST("Testing file/line/function")
{
    ReportingHandler handler;

    std::string file;
    int line = 0;
    std::string function;
    auto handling = [&](const LogData& data) {
        file = std::string(data.file);
        line = data.line;
        function = std::string(data.function);
    };

    handler.SetLoggingCallback(handling);

    LOG_INFO(handler, "Test");
    CHECK(file.find("ReportingHandlerTests.cpp") != std::string::npos);
    CHECK(line == (__LINE__ - 2));
    CHECK(function == "CATCH2_INTERNAL_TEST_4");

    LOG_WARNING(handler, "Test");
    CHECK(file.find("ReportingHandlerTests.cpp") != std::string::npos);
    CHECK(line == (__LINE__ - 2));
    CHECK(function == "CATCH2_INTERNAL_TEST_4");
}

TEST("Testing LogFormatting")
{
    ReportingHandler handler;

    std::string message;
    auto handling = [&](const LogData& data) { message = data.message; };

    handler.SetLoggingCallback(handling);

    REQUIRE(message.empty());

    LOG_INFO(handler, "Test %s", "Test");
    REQUIRE(message == "Test Test");

    LOG_WARNING(handler, "Test %s %s", "Test1", "Test2");
    REQUIRE(message == "Test Test1 Test2");

    LOG_ERROR(handler, "Test %s %s %s", "Test1", "Test2", "Test3");
    REQUIRE(message == "Test Test1 Test2 Test3");

    LOG_INFO(handler, "Test %d %d", 1, true);
    REQUIRE(message == "Test 1 1");

    LOG_INFO(handler, "Test %d %s", 2, false ? "true" : "false");
    REQUIRE(message == "Test 2 false");

    handler.SetLoggingCallback(nullptr);
}

TEST("Testing setting another logging callback waits for an existing call to finish")
{
    ReportingHandler handler;

    // Set a callback that will be blocked by a mutex
    std::mutex mutex;
    bool called = false;
    std::chrono::time_point<std::chrono::system_clock> time1;
    bool startedCall = false;
    auto handling = [&](const LogData& logData) {
        startedCall = true;
        std::lock_guard guard(mutex);
        called = true;
        time1 = logData.time;
    };

    handler.SetLoggingCallback(handling);

    // Make sure the callback is blocked
    std::unique_lock lock(mutex);

    // Spawn a thread that will be blocked by the callback
    std::thread t([&]() { LOG_INFO(handler, "Test"); });

    LoggingCallbackFn anotherHandling;
    SECTION("Setting another callback")
    {
        anotherHandling = [&](const LogData&) {};
    }
    SECTION("Setting a nullptr callback")
    {
        anotherHandling = nullptr;
    }

    // Spawn a second thread that tries to set another callback
    std::chrono::time_point<std::chrono::system_clock> time2;
    std::thread t2([&]() {
        // Make sure the callback has started and is now blocked
        while (!startedCall)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Now setting another callback should be blocked until we unlock the mutex
        handler.SetLoggingCallback(std::move(anotherHandling));
        INFO("The first callback should have been called at this point");
        REQUIRE(called);
        time2 = std::chrono::system_clock::now();
    });

    // Unlocking the mutex will allow the threads to continue
    lock.unlock();
    t.join();
    t2.join();

    INFO("The first callback should have been called before the second callback was set");
    REQUIRE(time1 < time2);
}

TEST("Testing ToString(LogSeverity)")
{
    REQUIRE(SFS::ToString(LogSeverity::Info) == "Info");
    REQUIRE(SFS::ToString(LogSeverity::Warning) == "Warning");
    REQUIRE(SFS::ToString(LogSeverity::Error) == "Error");
    REQUIRE(SFS::ToString(LogSeverity::Verbose) == "Verbose");
}
