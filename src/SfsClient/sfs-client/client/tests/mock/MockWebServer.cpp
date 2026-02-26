// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "MockWebServer.h"

#include "../util/TestHelper.h"
#include "ErrorHandling.h"
#include "ServerCommon.h"
#include "Util.h"
#include "connection/HttpHeader.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <nlohmann/json.hpp>

#include <chrono>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>

using namespace SFS;
using namespace SFS::details;
using namespace SFS::details::util;
using namespace SFS::test;
using namespace SFS::test::details;
using json = nlohmann::json;

namespace
{

struct App
{
    std::string version;
    std::vector<MockPrerequisite> prerequisites;
};

struct AppCmp
{
    bool operator()(App a, App b) const
    {
        return a.version < b.version;
    }
};

json GenerateContentIdJsonObject(const std::string& name, const std::string& latestVersion, const std::string& ns)
{
    // {
    //   "ContentId": {
    //     "Namespace": <ns>,
    //     "Name": <name>,
    //     "Version": <version>
    //   }
    // }

    return {{"ContentId", {{"Namespace", ns}, {"Name", name}, {"Version", latestVersion}}}};
}

json GenerateGetAppVersionJsonObject(const std::string& name, const App& app, const std::string& ns)
{
    // {
    //   "ContentId": {
    //     "Namespace": <ns>,
    //     "Name": <name>,
    //     "Version": <version>
    //   },
    //   "UpdateId": "<id>",
    //   "Prerequisites": [
    //     {
    //       "Namespace": "<ns>",
    //       "Name": "<name>",
    //       "Version": "<version>"
    //     }
    //   ]
    // }

    json prereqs = json::array();
    for (const auto& prereq : app.prerequisites)
    {
        prereqs.push_back({{"Namespace", ns}, {"Name", prereq.name}, {"Version", prereq.version}});
    }
    return {{"ContentId", {{"Namespace", ns}, {"Name", name}, {"Version", app.version}}},
            {"UpdateId", "123"},
            {"Prerequisites", prereqs}};
}

json GeneratePostDownloadInfo(const std::string& name)
{
    // [
    //   {
    //     "Url": <url>,
    //     "FileId": <fileid>,
    //     "SizeInBytes": <size>,
    //     "Hashes": {
    //       "Sha1": <sha1>,
    //       "Sha256": <sha2>
    //     },
    //     "DeliveryOptimization": {
    //       "CatalogId": <catalogid>,
    //       "Properties": {
    //         "IntegrityCheckInfo": {
    //           "PiecesHashFileUrl": <url>,
    //           "HashOfHashes": <hash>
    //         }
    //       }
    //     }
    //   },
    //   ...
    // ]

    // Generating DeliveryOptimizationData to simulate the server response, but it's not being parsed by the Client

    json response;
    response = json::array();
    response.push_back({{"Url", "http://localhost/1.json"},
                        {"FileId", name + ".json"},
                        {"SizeInBytes", 100},
                        {"Hashes", {{"Sha1", "123"}, {"Sha256", "456"}}}});
    response[0]["DeliveryOptimization"] = {{"CatalogId", "789"}};
    response[0]["DeliveryOptimization"]["Properties"] = {
        {"IntegrityCheckInfo", {{"PiecesHashFileUrl", "http://localhost/1.json"}, {"HashOfHashes", "abc"}}}};

    response.push_back({{"Url", "http://localhost/2.bin"},
                        {"FileId", name + ".bin"},
                        {"SizeInBytes", 200},
                        {"Hashes", {{"Sha1", "421"}, {"Sha256", "132"}}}});
    response[1]["DeliveryOptimization"] = {{"CatalogId", "14"}};
    response[1]["DeliveryOptimization"]["Properties"] = {
        {"IntegrityCheckInfo", {{"PiecesHashFileUrl", "http://localhost/2.bin"}, {"HashOfHashes", "abcd"}}}};
    return response;
}

json GeneratePostAppDownloadInfo(const std::string& name)
{
    // [
    //   {
    //     "Url": <url>,
    //     "FileId": <fileid>,
    //     "SizeInBytes": <size>,
    //     "Hashes": {
    //       "Sha1": <sha1>,
    //       "Sha256": <sha2>
    //     },
    //     "DeliveryOptimization": {
    //       "CatalogId": <catalogid>,
    //       "Properties": {
    //         "IntegrityCheckInfo": {
    //           "PiecesHashFileUrl": <url>,
    //           "HashOfHashes": <hash>
    //         }
    //       }
    //     },
    //     "ApplicabilityDetails": {
    //       "Architectures": [
    //         "<arch>"
    //       ],
    //       "PlatformApplicabilityForPackage": [
    //         "<app>"
    //       ]
    //     },
    //     "FileMoniker": "<moniker>"
    //   }
    // ]

    // Generating DeliveryOptimizationData to simulate the server response, but it's not being parsed by the Client

    json response;
    response = json::array();
    response.push_back({{"Url", "http://localhost/1.json"},
                        {"FileId", name + ".json"},
                        {"SizeInBytes", 100},
                        {"Hashes", {{"Sha1", "123"}, {"Sha256", "456"}}}});
    response[0]["DeliveryOptimization"] = {{"CatalogId", "789"}};
    response[0]["DeliveryOptimization"]["Properties"] = {
        {"IntegrityCheckInfo", {{"PiecesHashFileUrl", "http://localhost/1.json"}, {"HashOfHashes", "abc"}}}};
    response[0]["ApplicabilityDetails"] = {{"Architectures", {"x86"}},
                                           {"PlatformApplicabilityForPackage", {"Windows"}}};
    response[0]["FileMoniker"] = "1.json";

    response.push_back({{"Url", "http://localhost/2.bin"},
                        {"FileId", name + ".bin"},
                        {"SizeInBytes", 200},
                        {"Hashes", {{"Sha1", "421"}, {"Sha256", "132"}}}});
    response[1]["DeliveryOptimization"] = {{"CatalogId", "14"}};
    response[1]["DeliveryOptimization"]["Properties"] = {
        {"IntegrityCheckInfo", {{"PiecesHashFileUrl", "http://localhost/2.bin"}, {"HashOfHashes", "abcd"}}}};
    response[1]["ApplicabilityDetails"] = {{"Architectures", {"amd64"}},
                                           {"PlatformApplicabilityForPackage", {"Linux"}}};
    response[1]["FileMoniker"] = "2.bin";

    return response;
}

void CheckApiVersion(const httplib::Request& req, std::string_view apiVersion)
{
    if (util::AreNotEqualI(req.path_params.at("apiVersion"), apiVersion))
    {
        throw StatusCodeException(httplib::StatusCode::NotFound_404);
    }
}
} // namespace

namespace SFS::test::details
{
class MockWebServerImpl : public BaseServerImpl
{
  public:
    MockWebServerImpl() = default;
    ~MockWebServerImpl() = default;

    MockWebServerImpl(const MockWebServerImpl&) = delete;
    MockWebServerImpl& operator=(const MockWebServerImpl&) = delete;

    void RegisterProduct(std::string&& name, std::string&& version);
    void RegisterAppProduct(std::string&& name, std::string&& version, std::vector<MockPrerequisite>&& prerequisites);
    void RegisterExpectedRequestHeader(std::string&& header, std::string&& value);
    void SetForcedHttpErrors(std::queue<HttpCode> forcedErrors);
    void SetResponseHeaders(std::unordered_map<HttpCode, HeaderMap> headersByCode);

  private:
    void ConfigureRequestHandlers() override;
    std::string GetLogIdentifier() override;

    void ConfigurePostLatestVersion();
    void ConfigurePostLatestVersionBatch();
    void ConfigureGetSpecificVersion();
    void ConfigurePostDownloadInfo();

    void RunHttpCallback(const httplib::Request& req,
                         httplib::Response& res,
                         const std::string& methodName,
                         const std::string& apiVersion,
                         const std::function<void(const httplib::Request, httplib::Response&)>& callback);
    void CheckRequestHeaders(const httplib::Request& req);

    using VersionList = std::set<std::string>;
    std::unordered_map<std::string, VersionList> m_products;

    using AppList = std::set<App, AppCmp>;
    std::unordered_map<std::string, AppList> m_appProducts;

    std::unordered_map<std::string, std::string> m_expectedRequestHeaders;
    std::queue<HttpCode> m_forcedHttpErrors;
    std::unordered_map<HttpCode, HeaderMap> m_headersByCode;
};
} // namespace SFS::test::details

MockWebServer::MockWebServer()
{
    m_impl = std::make_unique<MockWebServerImpl>();
    m_impl->Start();
}

MockWebServer::~MockWebServer()
{
    const auto ret = Stop();
    if (!ret)
    {
        TEST_UNSCOPED_INFO("Failed to stop: " + std::string(ToString(ret.GetCode())));
    }
}

Result MockWebServer::Stop()
{
    return m_impl->Stop();
}

std::string MockWebServer::GetBaseUrl() const
{
    return m_impl->GetUrl();
}

void MockWebServer::RegisterProduct(std::string name, std::string version)
{
    m_impl->RegisterProduct(std::move(name), std::move(version));
}

void MockWebServer::RegisterAppProduct(std::string name,
                                       std::string version,
                                       std::vector<MockPrerequisite> prerequisites)
{
    m_impl->RegisterAppProduct(std::move(name), std::move(version), std::move(prerequisites));
}

void MockWebServer::RegisterExpectedRequestHeader(HttpHeader header, std::string value)
{
    std::string headerName = ToString(header);
    m_impl->RegisterExpectedRequestHeader(std::move(headerName), std::move(value));
}

void MockWebServer::SetForcedHttpErrors(std::queue<HttpCode> forcedErrors)
{
    m_impl->SetForcedHttpErrors(std::move(forcedErrors));
}

void MockWebServer::SetResponseHeaders(std::unordered_map<HttpCode, HeaderMap> headersByCode)
{
    m_impl->SetResponseHeaders(std::move(headersByCode));
}

void MockWebServerImpl::ConfigureRequestHandlers()
{
    ConfigurePostLatestVersion();
    ConfigurePostLatestVersionBatch();
    ConfigureGetSpecificVersion();
    ConfigurePostDownloadInfo();
}

std::string MockWebServerImpl::GetLogIdentifier()
{
    return "MockWebServer";
}

void MockWebServerImpl::ConfigurePostLatestVersion()
{
    // Path: /api/<apiVersion:v2>/contents/<instanceId>/namespaces/<ns>/names/<name>/versions/latest?action=select
    const std::string pattern = "/api/:apiVersion/contents/:instanceId/namespaces/:ns/names/:name/versions/latest";
    m_server.Post(pattern, [&](const httplib::Request& req, httplib::Response& res) {
        RunHttpCallback(req, res, "PostLatestVersion", "v2", [&](const httplib::Request& req, httplib::Response& res) {
            // TODO: Ignoring instanceId for now

            if (!req.has_param("action") || util::AreNotEqualI(req.get_param_value("action"), "select"))
            {
                // TODO: SFS might throw a different error when the query string is unexpected
                throw StatusCodeException(httplib::StatusCode::NotFound_404);
            }

            // Checking body has expected format, but won't use it for the response
            {
                if (req.body.empty())
                {
                    throw StatusCodeException(httplib::StatusCode::BadRequest_400);
                }

                json body;
                try
                {
                    body = json::parse(req.body);
                }
                catch (const json::parse_error& ex)
                {
                    BUFFER_LOG("JSON parse error: " + std::string(ex.what()));
                    throw StatusCodeException(httplib::StatusCode::BadRequest_400);
                }

                // The GetLatestVersion API expects an object as a body, with a "TargetingAttributes" object element.
                if (!body.is_object() || !body.contains("TargetingAttributes") ||
                    !body["TargetingAttributes"].is_object())
                {
                    throw StatusCodeException(httplib::StatusCode::BadRequest_400);
                }
            }

            const std::string ns = req.path_params.at("ns");

            json response;
            const std::string& name = req.path_params.at("name");
            if (auto it = m_products.find(name); it != m_products.end())
            {
                const auto& versions = it->second;
                if (versions.empty())
                {
                    throw StatusCodeException(httplib::StatusCode::InternalServerError_500);
                }

                const auto& latestVersion = *versions.rbegin();
                response = GenerateContentIdJsonObject(name, latestVersion, ns);
            }
            else if (auto appIt = m_appProducts.find(name); appIt != m_appProducts.end())
            {
                const auto& appList = appIt->second;
                if (appList.empty())
                {
                    throw StatusCodeException(httplib::StatusCode::InternalServerError_500);
                }

                const auto& latestApp = *appList.rbegin();
                response = GenerateGetAppVersionJsonObject(name, latestApp, ns);
            }
            else
            {
                throw StatusCodeException(httplib::StatusCode::NotFound_404);
            }

            res.set_content(response.dump(), "application/json");
        });
    });
}

void MockWebServerImpl::ConfigurePostLatestVersionBatch()
{
    // Path: /api/<apiVersion:v2>/contents/<instanceId>/namespaces/<ns>/names?action=BatchUpdates
    const std::string pattern = "/api/:apiVersion/contents/:instanceId/namespaces/:ns/names";
    m_server.Post(pattern, [&](const httplib::Request& req, httplib::Response& res) {
        RunHttpCallback(
            req,
            res,
            "PostLatestVersionBatch",
            "v2",
            [&](const httplib::Request& req, httplib::Response& res) {
                // TODO: Ignoring instanceId for now

                if (!req.has_param("action") || util::AreNotEqualI(req.get_param_value("action"), "BatchUpdates"))
                {
                    // TODO: SFS might throw a different error when the query string is unexpected
                    throw StatusCodeException(httplib::StatusCode::NotFound_404);
                }

                if (req.body.empty())
                {
                    throw StatusCodeException(httplib::StatusCode::BadRequest_400);
                }

                json body;
                try
                {
                    body = json::parse(req.body);
                }
                catch (const json::parse_error& ex)
                {
                    BUFFER_LOG("JSON parse error: " + std::string(ex.what()));
                    throw StatusCodeException(httplib::StatusCode::BadRequest_400);
                }

                // The BatchUpdates API returns an array of objects, each with a "Product" key.
                // If repeated, the same product is only returned once.
                // TODO: We are ignoring the TargetingAttributes for now.
                if (!body.is_array())
                {
                    throw StatusCodeException(httplib::StatusCode::BadRequest_400);
                }

                // Iterate over the array and collect the unique products
                std::unordered_map<std::string, json> requestedProducts;
                for (const auto& productRequest : body)
                {
                    if (!productRequest.is_object() || !productRequest.contains("Product") ||
                        !productRequest["Product"].is_string() || !productRequest.contains("TargetingAttributes"))
                    {
                        throw StatusCodeException(httplib::StatusCode::BadRequest_400);
                    }
                    if (requestedProducts.count(productRequest["Product"]))
                    {
                        continue;
                    }
                    requestedProducts.emplace(productRequest["Product"], productRequest["TargetingAttributes"]);
                }

                // If at least one product exists, we will return a 200 OK with that. Non-existing products are ignored.
                // Otherwise, a 404 is sent.
                json response = json::array();
                for (const auto& [name, _] : requestedProducts)
                {
                    auto it = m_products.find(name);
                    if (it == m_products.end())
                    {
                        continue;
                    }

                    const VersionList& versions = it->second;
                    if (versions.empty())
                    {
                        throw StatusCodeException(httplib::StatusCode::InternalServerError_500);
                    }

                    const std::string ns = req.path_params.at("ns");
                    const auto& latestVersion = *versions.rbegin();

                    response.push_back(GenerateContentIdJsonObject(name, latestVersion, ns));
                }

                if (response.empty())
                {
                    throw StatusCodeException(httplib::StatusCode::NotFound_404);
                }

                res.set_content(response.dump(), "application/json");
            });
    });
}

void MockWebServerImpl::ConfigureGetSpecificVersion()
{
    // Path: /api/<apiVersion:v2>/contents/<instanceId>/namespaces/<ns>/names/<name>/versions/<version>
    const std::string pattern = "/api/:apiVersion/contents/:instanceId/namespaces/:ns/names/:name/versions/:version";
    m_server.Get(pattern, [&](const httplib::Request& req, httplib::Response& res) {
        RunHttpCallback(req, res, "GetSpecificVersion", "v2", [&](const httplib::Request& req, httplib::Response& res) {
            // TODO: Ignoring instanceId for now

            const std::string& name = req.path_params.at("name");
            auto it = m_products.find(name);
            if (it == m_products.end())
            {
                throw StatusCodeException(httplib::StatusCode::NotFound_404);
            }

            const VersionList& versions = it->second;
            if (versions.empty())
            {
                throw StatusCodeException(httplib::StatusCode::InternalServerError_500);
            }

            // TODO: Are apps suppported?

            const std::string& version = req.path_params.at("version");
            if (version.empty() || !versions.count(version))
            {
                throw StatusCodeException(httplib::StatusCode::NotFound_404);
            }

            const std::string ns = req.path_params.at("ns");

            res.set_content(GenerateContentIdJsonObject(name, version, ns).dump(), "application/json");
        });
    });
}

void MockWebServerImpl::ConfigurePostDownloadInfo()
{
    // Path:
    // /api/<apiVersion:v2>/contents/<instanceId>/namespaces/<ns>/names/<name>/versions/<version>/files?action=GenerateDownloadInfo
    const std::string pattern =
        "/api/:apiVersion/contents/:instanceId/namespaces/:ns/names/:name/versions/:version/files";
    m_server.Post(pattern, [&](const httplib::Request& req, httplib::Response& res) {
        RunHttpCallback(req, res, "PostDownloadInfo", "v2", [&](const httplib::Request& req, httplib::Response& res) {
            // TODO: Ignoring instanceId and ns for now

            if (!req.has_param("action") || util::AreNotEqualI(req.get_param_value("action"), "GenerateDownloadInfo"))
            {
                // TODO: SFS might throw a different error when the query string is unexpected
                throw StatusCodeException(httplib::StatusCode::NotFound_404);
            }

            const std::string& version = req.path_params.at("version");
            if (version.empty())
            {
                throw StatusCodeException(httplib::StatusCode::NotFound_404);
            }

            json response;
            const std::string& name = req.path_params.at("name");
            if (auto it = m_products.find(name); it != m_products.end())
            {
                const auto& versions = it->second;
                if (versions.empty())
                {
                    throw StatusCodeException(httplib::StatusCode::InternalServerError_500);
                }

                if (!versions.count(version))
                {
                    throw StatusCodeException(httplib::StatusCode::NotFound_404);
                }

                // Response is a dummy, doesn't use the version above
                response = GeneratePostDownloadInfo(name);
            }
            else if (auto appIt = m_appProducts.find(name); appIt != m_appProducts.end())
            {
                const auto& appList = appIt->second;
                if (appList.empty())
                {
                    throw StatusCodeException(httplib::StatusCode::InternalServerError_500);
                }

                auto app = std::find_if(appList.begin(), appList.end(), [&](const App& app) {
                    return app.version == version;
                });
                if (app == appList.end())
                {
                    throw StatusCodeException(httplib::StatusCode::NotFound_404);
                }

                // Response is a dummy, doesn't use the version above
                response = GeneratePostAppDownloadInfo(name);
            }
            else
            {
                throw StatusCodeException(httplib::StatusCode::NotFound_404);
            }

            res.set_content(response.dump(), "application/json");
        });
    });
}

void MockWebServerImpl::RunHttpCallback(const httplib::Request& req,
                                        httplib::Response& res,
                                        const std::string& methodName,
                                        const std::string& apiVersion,
                                        const std::function<void(const httplib::Request, httplib::Response&)>& callback)
{
    if (m_forcedHttpErrors.size() > 0)
    {
        res.status = m_forcedHttpErrors.front();
        m_forcedHttpErrors.pop();

        BUFFER_LOG("Forcing HTTP error: " + std::to_string(res.status));
    }
    else
    {
        try
        {
            BUFFER_LOG("Matched " + methodName);
            CheckApiVersion(req, apiVersion);
            CheckRequestHeaders(req);
            callback(req, res);
            res.status = httplib::StatusCode::OK_200;
        }
        catch (const StatusCodeException& ex)
        {
            res.status = ex.GetStatusCode();
        }
        catch (const std::exception&)
        {
            res.status = httplib::StatusCode::InternalServerError_500;
        }
        catch (...)
        {
            res.status = httplib::StatusCode::InternalServerError_500;
        }
    }

    if (m_headersByCode.count(res.status) > 0)
    {
        BUFFER_LOG("HTTP code " + std::to_string(res.status) + " has response headers to be sent");
        for (const auto& header : m_headersByCode[res.status])
        {
            BUFFER_LOG("Adding header [" + header.first + "] with value [" + header.second + "]");
            res.set_header(header.first, header.second);
        }
    }
}

void MockWebServerImpl::CheckRequestHeaders(const httplib::Request& req)
{
    for (const auto& header : m_expectedRequestHeaders)
    {
        std::optional<std::string> errorMessage;
        if (!req.has_header(header.first))
        {
            errorMessage = "Expected header [" + header.first + "] not found";
        }
        else if (util::AreNotEqualI(req.get_header_value(header.first), header.second))
        {
            errorMessage = "Header [" + header.first + "] with value [" + req.get_header_value(header.first) +
                           "] does not match the expected value [" + header.second + "]";
        }

        if (errorMessage)
        {
            BUFFER_LOG(*errorMessage);
            throw std::runtime_error(errorMessage->c_str());
        }
    }
}

void MockWebServerImpl::RegisterProduct(std::string&& name, std::string&& version)
{
    m_products[std::move(name)].emplace(std::move(version));
}

void MockWebServerImpl::RegisterAppProduct(std::string&& name,
                                           std::string&& version,
                                           std::vector<MockPrerequisite>&& prerequisites)
{
    for (const auto& prereq : prerequisites)
    {
        m_appProducts[prereq.name].emplace(App{prereq.version, {}});
    }
    m_appProducts[std::move(name)].emplace(App{std::move(version), std::move(prerequisites)});
}

void MockWebServerImpl::RegisterExpectedRequestHeader(std::string&& header, std::string&& value)
{
    if (auto it = m_expectedRequestHeaders.find(header); it != m_expectedRequestHeaders.end())
    {
        it->second = std::move(value);
        return;
    }
    else
    {
        m_expectedRequestHeaders.emplace(std::move(header), std::move(value));
    }
}

void MockWebServerImpl::SetForcedHttpErrors(std::queue<int> forcedErrors)
{
    m_forcedHttpErrors = std::move(forcedErrors);
}

void MockWebServerImpl::SetResponseHeaders(std::unordered_map<HttpCode, HeaderMap> headersByCode)
{
    m_headersByCode = std::move(headersByCode);
}
