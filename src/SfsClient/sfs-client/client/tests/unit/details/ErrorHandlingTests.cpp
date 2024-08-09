// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ErrorHandling.h"
#include "ReportingHandler.h"
#include "Result.h"
#include "SFSException.h"

#include <catch2/catch_test_macros.hpp>

#define TEST(...) TEST_CASE("[ErrorHandlingTests] " __VA_ARGS__)

using namespace SFS;
using namespace SFS::details;

namespace
{
Result TestSFS_Catch_Return_bad_alloc()
try
{
    throw std::bad_alloc();
}
SFS_CATCH_RETURN();

class MyException : public std::exception
{
};

Result TestSFS_Catch_Return_std_exception()
try
{
    throw MyException();
}
SFS_CATCH_RETURN();

Result TestSFS_Catch_Return_unknown()
try
{
    throw std::error_code();
}
SFS_CATCH_RETURN();
} // namespace

TEST("Testing ErrorHandling's SFS_CATCH_RETURN()")
{
    REQUIRE(TestSFS_Catch_Return_bad_alloc() == Result::OutOfMemory);
    REQUIRE(TestSFS_Catch_Return_std_exception() == Result::Unexpected);
    REQUIRE(TestSFS_Catch_Return_unknown() == Result::Unexpected);
}

namespace
{
void TestSFS_Catch_Log_Rethrow_bad_alloc(const ReportingHandler& handler)
try
{
    throw std::bad_alloc();
}
SFS_CATCH_LOG_RETHROW(handler);

void TestSFS_Catch_Log_Rethrow_SFSException(const ReportingHandler& handler)
try
{
    throw SFS::details::SFSException(Result::Unexpected);
}
SFS_CATCH_LOG_RETHROW(handler);
} // namespace

TEST("Testing ErrorHandling's SFS_CATCH_LOG_RETHROW()")
{
    ReportingHandler handler;
    bool called = false;
    handler.SetLoggingCallback([&](const auto&) { called = true; });

    SECTION("Test that SFS_CATCH_LOG_RETHROW does not rethrow if not SFSException")
    {
        REQUIRE_THROWS_AS(TestSFS_Catch_Log_Rethrow_bad_alloc(handler), std::bad_alloc);
        REQUIRE(!called);
    }

    SECTION("Test that SFS_CATCH_LOG_RETHROW rethrows and logs if SFSException")
    {
        REQUIRE_THROWS_AS(TestSFS_Catch_Log_Rethrow_SFSException(handler), SFSException);
        REQUIRE(called);
    }
}

namespace
{
Result TestSFS_ReturnIfFailed(const Result& result, const Result& ifNotFailed = Result::Success)
{
    RETURN_IF_FAILED(result);
    return ifNotFailed;
}
} // namespace

TEST("Testing ErrorHandling's RETURN_IF_FAILED()")
{
    SECTION("Test that RETURN_IF_FAILED returns the result if it is a failure")
    {
        REQUIRE(TestSFS_ReturnIfFailed(Result::Code::Unexpected) == Result::Code::Unexpected);
    }

    SECTION("Test that RETURN_IF_FAILED does not return if the result is a success")
    {
        REQUIRE(TestSFS_ReturnIfFailed(Result::Code::Success) == Result::Code::Success);
        REQUIRE(TestSFS_ReturnIfFailed(Result::Code::Success, Result::Code::Unexpected) == Result::Code::Unexpected);
    }
}

namespace
{
Result TestSFS_ReturnIfFailedLog(const ReportingHandler& handler,
                                 const Result& result,
                                 const Result& ifNotFailed = Result::Success)
{
    RETURN_IF_FAILED_LOG(result, handler);
    return ifNotFailed;
}
} // namespace

TEST("Testing ErrorHandling's RETURN_IF_FAILED_LOG()")
{
    ReportingHandler handler;
    bool called = false;
    handler.SetLoggingCallback([&](const auto&) { called = true; });

    SECTION("Test that RETURN_IF_FAILED_LOG returns and logs if the result is a failure")
    {
        REQUIRE(TestSFS_ReturnIfFailedLog(handler, Result::Code::Unexpected) == Result::Code::Unexpected);
        REQUIRE(called);
    }

    SECTION("Test that RETURN_IF_FAILED_LOG does not return and log if the result is a success")
    {
        REQUIRE(TestSFS_ReturnIfFailedLog(handler, Result::Code::Success) == Result::Code::Success);
        REQUIRE(!called);
    }
}

TEST("Testing ErrorHandling's LOG_IF_FAILED()")
{
    ReportingHandler handler;
    bool called = false;
    handler.SetLoggingCallback([&](const auto&) { called = true; });

    SECTION("Test that LOG_IF_FAILED logs if the result is a failure")
    {
        LOG_IF_FAILED(Result(Result::Unexpected), handler);
        REQUIRE(called);
    }

    SECTION("Test that LOG_IF_FAILED does not log if the result is a success")
    {
        LOG_IF_FAILED(Result(Result::Success), handler);
        REQUIRE(!called);
    }
}

namespace
{
void TestSFS_ThrowLog(const ReportingHandler& handler, const Result& result)
{
    THROW_LOG(result, handler);
}
} // namespace

TEST("Testing ErrorHandling's THROW_LOG()")
{
    INFO("Test that ErrorHandling's THROW_LOG throws and logs the result");

    ReportingHandler handler;
    bool called = false;
    handler.SetLoggingCallback([&](const auto&) { called = true; });

    REQUIRE_THROWS_AS(TestSFS_ThrowLog(handler, Result::Code::Unexpected), SFSException);
    REQUIRE(called);
}

namespace
{
void TestSFS_ThrowIfFailedLog(const ReportingHandler& handler, const Result& result)
{
    THROW_IF_FAILED_LOG(result, handler);
}
} // namespace

TEST("Testing ErrorHandling's THROW_IF_FAILED_LOG()")
{
    ReportingHandler handler;
    bool called = false;
    handler.SetLoggingCallback([&](const auto&) { called = true; });

    SECTION("Test that ErrorHandling's THROW_IF_FAILED_LOG throws and logs the result if it is a failure")
    {
        REQUIRE_THROWS_AS(TestSFS_ThrowIfFailedLog(handler, Result::Code::Unexpected), SFSException);
        REQUIRE(called);
    }

    SECTION("Test that ErrorHandling's THROW_IF_FAILED_LOG does not throw and log the result if it is not a failure")
    {
        REQUIRE_NOTHROW(TestSFS_ThrowIfFailedLog(handler, Result::Code::Success));
        REQUIRE(!called);
    }
}

TEST("Testing ErrorHandling's THROW_CODE_IF()")
{
    SECTION("Test that THROW_CODE_IF throws if the condition is true")
    {
        REQUIRE_THROWS_AS([]() { THROW_CODE_IF(Unexpected, true); }(), SFSException);
    }

    SECTION("Test that THROW_CODE_IF does not throw if the condition is false")
    {
        REQUIRE_NOTHROW([]() { THROW_CODE_IF(Unexpected, false); }());
    }
}

TEST("Testing ErrorHandling's THROW_CODE_IF_LOG()")
{
    ReportingHandler handler;
    bool called = false;
    handler.SetLoggingCallback([&](const auto&) { called = true; });

    SECTION("Test that THROW_CODE_IF_LOG throws and logs the result if the condition is true")
    {
        REQUIRE_THROWS_AS([&handler]() { THROW_CODE_IF_LOG(Unexpected, true, handler); }(), SFSException);
        REQUIRE(called);
    }

    SECTION("Test that THROW_CODE_IF_LOG does not throw and log if the condition is false")
    {
        REQUIRE_NOTHROW([&handler]() { THROW_CODE_IF_LOG(Unexpected, false, handler); }());
        REQUIRE(!called);
    }
}

TEST("Testing ErrorHandling's THROW_CODE_IF_NOT_LOG()")
{
    ReportingHandler handler;
    bool called = false;
    handler.SetLoggingCallback([&](const auto&) { called = true; });

    SECTION("Test that THROW_CODE_IF_NOT_LOG throws and logs the result if the condition is false")
    {
        REQUIRE_THROWS_AS([&handler]() { THROW_CODE_IF_NOT_LOG(Unexpected, false, handler); }(), SFSException);
        REQUIRE(called);
    }

    SECTION("Test that THROW_CODE_IF_NOT_LOG does not throw and log if the condition is true")
    {
        REQUIRE_NOTHROW([&handler]() { THROW_CODE_IF_NOT_LOG(Unexpected, true, handler); }());
        REQUIRE(!called);
    }
}
