// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ProxyServer.h"

#include "../util/TestHelper.h"
#include "ErrorHandling.h"
#include "ReportingHandler.h"
#include "ServerCommon.h"
#include "UrlBuilder.h"

using namespace SFS;
using namespace SFS::details;
using namespace SFS::test;
using namespace SFS::test::details;

namespace SFS::test::details
{
class ProxyServerImpl : public BaseServerImpl
{
  private:
    void ConfigureRequestHandlers() override;
    std::string GetLogIdentifier() override;
};
} // namespace SFS::test::details

ProxyServer::ProxyServer()
{
    m_impl = std::make_unique<ProxyServerImpl>();
    m_impl->Start();
}

ProxyServer::~ProxyServer()
{
    const auto ret = Stop();
    if (!ret)
    {
        TEST_UNSCOPED_INFO("Failed to stop: " + std::string(ToString(ret.GetCode())));
    }
}

Result ProxyServer::Stop()
{
    return m_impl->Stop();
}

std::string ProxyServer::GetBaseUrl() const
{
    return m_impl->GetUrl();
}

void ProxyServerImpl::ConfigureRequestHandlers()
{
    auto HandleRequest = [&](const httplib::Request& req, httplib::Response& res) {
        // As a proxy, we'll parse the URL and Path/Query so we can reuse them in httplib::Client
        ReportingHandler handler;
        UrlBuilder urlBuilder(req.target.c_str(), handler);
        const std::string path = urlBuilder.GetPath();
        const std::string query = urlBuilder.GetQuery();
        urlBuilder.ResetPath().ResetQuery();

        // URL may come back from UrlBuilder with a final /, which doesn't work with httplib::Client, so we remove it
        auto url = urlBuilder.GetUrl();
        if (url.at(url.size() - 1) == '/')
        {
            url.pop_back();
        }
        const std::string pathAndQuery = path + (query.empty() ? "" : ("?" + query));
        httplib::Client cli(url);
        httplib::Result result;
        if (req.method == "GET")
        {
            result = cli.Get(pathAndQuery, req.headers);
        }
        else if (req.method == "POST")
        {
            const auto length = std::stoi(req.get_header_value("Content-Length"));
            result =
                cli.Post(pathAndQuery, req.headers, req.body.c_str(), length, req.get_header_value("Content-Type"));
        }

        if (!result)
        {
            BUFFER_LOG("Client error: " + to_string(result.error()));
            throw StatusCodeException(httplib::StatusCode::InternalServerError_500);
        }
        res = result.value();
    };

    m_server.Get(".*", HandleRequest);
    m_server.Post(".*", HandleRequest);
}

std::string ProxyServerImpl::GetLogIdentifier()
{
    return "ProxyServer";
}
