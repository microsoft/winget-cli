// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "../../util/SFSExceptionMatcher.h"
#include "../../util/TestHelper.h"
#include "ReportingHandler.h"
#include "Result.h"
#include "connection/CurlConnection.h"
#include "connection/mock/MockConnection.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <nlohmann/json.hpp>

#define TEST(...) TEST_CASE("[CurlConnectionTests] " __VA_ARGS__)

using namespace SFS;
using namespace SFS::details;
using namespace SFS::test;
using json = nlohmann::json;

namespace
{
class MockCurlConnection : public CurlConnection
{
  public:
    MockCurlConnection(const ReportingHandler& handler, Result::Code& responseCode, std::string& response)
        : CurlConnection({}, handler)
        , m_responseCode(responseCode)
        , m_response(response)
    {
    }

  protected:
    std::string CurlPerform(const std::string&, CurlHeaderList&) override
    {
        if (m_responseCode == Result::Success)
        {
            return m_response;
        }
        throw SFSException(m_responseCode);
    }

  private:
    Result::Code& m_responseCode;
    std::string& m_response;
};
} // namespace

TEST("Testing CurlConnection()")
{
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);
    Result::Code responseCode = Result::HttpNotFound;
    std::string response = "expected";
    std::unique_ptr<Connection> connection = std::make_unique<MockCurlConnection>(handler, responseCode, response);

    SECTION("Testing CurlConnection::Get()")
    {
        std::string out;
        REQUIRE_THROWS_CODE(out = connection->Get("url"), HttpNotFound);
        REQUIRE(out.empty());

        responseCode = Result::Success;
        REQUIRE_NOTHROW(out = connection->Get("url"));
        REQUIRE(out == response);
    }

    SECTION("Testing CurlConnection::Post()")
    {
        std::string out;
        REQUIRE_THROWS_CODE(out = connection->Post("url"), HttpNotFound);
        REQUIRE(out.empty());

        const json body = {{{"dummy", {}}}};
        REQUIRE_THROWS_CODE(out = connection->Post("url", body.dump()), HttpNotFound);
        REQUIRE(out.empty());

        responseCode = Result::Success;
        REQUIRE_NOTHROW(out = connection->Get("url"));
        REQUIRE(out == response);

        response.clear();
        REQUIRE_NOTHROW(out = connection->Post("url", body.dump()));
        REQUIRE(out == response);
    }
}

TEST("Testing CurlConnection constructor passing a cv")
{
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);

    ConnectionConfig config;
    config.baseCV = "";

    REQUIRE_THROWS_CODE_MSG(std::make_unique<CurlConnection>(config, handler), InvalidArg, "cv must not be empty");

    config.baseCV = "cv";
    REQUIRE_THROWS_CODE_MSG_MATCHES(std::make_unique<CurlConnection>(config, handler),
                                    InvalidArg,
                                    Catch::Matchers::ContainsSubstring("baseCV is not a valid correlation vector:"));

    config.baseCV = "aaaaaaaaaaaaaaaa.1";
    REQUIRE_NOTHROW(std::make_unique<CurlConnection>(config, handler));
}
