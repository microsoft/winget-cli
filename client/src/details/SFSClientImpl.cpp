// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "SFSClientImpl.h"

#include "AppContent.h"
#include "Content.h"
#include "ErrorHandling.h"
#include "Logging.h"
#include "TestOverride.h"
#include "Util.h"
#include "connection/Connection.h"
#include "connection/ConnectionManager.h"
#include "connection/CurlConnectionManager.h"
#include "connection/mock/MockConnectionManager.h"

#include <nlohmann/json.hpp>

#include <unordered_set>

using namespace SFS;
using namespace SFS::details;
using namespace SFS::details::util;
using json = nlohmann::json;

constexpr const char* c_defaultInstanceId = "default";
constexpr const char* c_defaultNameSpace = "default";

namespace
{
void ValidateClientConfig(const ClientConfig& config, const ReportingHandler& handler)
{
    THROW_CODE_IF_LOG(InvalidArg, config.accountId.empty(), handler, "ClientConfig::accountId must not be empty");

    if (config.instanceId)
    {
        THROW_CODE_IF_LOG(InvalidArg,
                          config.instanceId->empty(),
                          handler,
                          "ClientConfig::instanceId must not be empty");
    }

    if (config.nameSpace)
    {
        THROW_CODE_IF_LOG(InvalidArg, config.nameSpace->empty(), handler, "ClientConfig::nameSpace must not be empty");
    }
}

void LogIfTestOverridesAllowed(const ReportingHandler& handler)
{
    if (test::AreTestOverridesAllowed())
    {
        LOG_INFO(handler, "Test overrides are allowed");
    }
}

void ThrowInvalidResponseIfFalse(bool condition, const std::string& message, const ReportingHandler& handler)
{
    THROW_CODE_IF_LOG(ServiceInvalidResponse, !condition, handler, message);
}

json ParseServerMethodStringToJson(const std::string& data, const std::string& method, const ReportingHandler& handler)
{
    try
    {
        return json::parse(data);
    }
    catch (json::parse_error& ex)
    {
        THROW_LOG(
            Result(Result::ServiceInvalidResponse, "(" + method + ") JSON Parsing error: " + std::string(ex.what())),
            handler);
        return json(); // Unreachable code, but the compiler doesn't know that.
    }
}

VersionEntities ConvertLatestVersionBatchResponseToVersionEntities(const json& data, const ReportingHandler& handler)
{
    // Expected format:
    // [
    //   {
    //     "ContentId": {
    //       "Namespace": <ns>,
    //       "Name": <name>,
    //       "Version": <version>
    //     }
    //   },
    //   ...
    // ]
    //

    ThrowInvalidResponseIfFalse(data.is_array(), "Response is not a JSON array", handler);
    ThrowInvalidResponseIfFalse(data.size() > 0, "Response does not have the expected size", handler);

    VersionEntities entities;
    for (const auto& obj : data)
    {
        entities.push_back(std::move(VersionEntity::FromJson(obj, handler)));
    }

    return entities;
}

bool VerifyVersionResponseMatchesProduct(const ContentIdEntity& contentId,
                                         std::string_view nameSpace,
                                         std::string_view name)
{
    return contentId.nameSpace == nameSpace && contentId.name == name;
}

void ValidateVersionEntity(const VersionEntity& versionEntity,
                           const std::string& nameSpace,
                           const std::string& product,
                           const ReportingHandler& handler)
{
    THROW_CODE_IF_NOT_LOG(ServiceInvalidResponse,
                          VerifyVersionResponseMatchesProduct(versionEntity.contentId, nameSpace, product),
                          handler,
                          "Response does not match the requested product");
}

void ValidateBatchVersionEntity(const VersionEntities& versionEntities,
                                const std::string& nameSpace,
                                const std::unordered_set<std::string>& requestedProducts,
                                const ReportingHandler& handler)
{
    for (const auto& entity : versionEntities)
    {
        THROW_CODE_IF_LOG(ServiceInvalidResponse,
                          requestedProducts.count(entity->contentId.name) == 0,
                          handler,
                          "Received product [" + entity->contentId.name +
                              "] which is not one of the requested products");
        THROW_CODE_IF_LOG(ServiceInvalidResponse,
                          AreNotEqualI(entity->contentId.nameSpace, nameSpace),
                          handler,
                          "Received product [" + entity->contentId.name + "] with a namespace [" +
                              entity->contentId.nameSpace + "] that does not match the requested namespace");

        LOG_INFO(handler,
                 "Received a response for product [%s] with version %s",
                 entity->contentId.name.c_str(),
                 entity->contentId.version.c_str());
    }
}

void ValidateRequestParams(const RequestParams& requestParams, const ReportingHandler& handler)
{
    THROW_CODE_IF_LOG(InvalidArg, requestParams.productRequests.empty(), handler, "productRequests cannot be empty");

    // TODO #78: Add support for multiple product requests
    THROW_CODE_IF_LOG(NotImpl,
                      requestParams.productRequests.size() > 1,
                      handler,
                      "There cannot be more than 1 productRequest at the moment");

    for (const auto& [product, _] : requestParams.productRequests)
    {
        THROW_CODE_IF_LOG(InvalidArg, product.empty(), handler, "product must not be empty");
    }
}
} // namespace

template <typename ConnectionManagerT>
SFSClientImpl<ConnectionManagerT>::SFSClientImpl(ClientConfig&& config)
{
    if (config.logCallbackFn)
    {
        m_reportingHandler.SetLoggingCallback(std::move(*config.logCallbackFn));
    }

    ValidateClientConfig(config, m_reportingHandler);

    m_accountId = std::move(config.accountId);
    m_instanceId =
        (config.instanceId && !config.instanceId->empty()) ? std::move(*config.instanceId) : c_defaultInstanceId;
    m_nameSpace = (config.nameSpace && !config.nameSpace->empty()) ? std::move(*config.nameSpace) : c_defaultNameSpace;

    static_assert(std::is_base_of<ConnectionManager, ConnectionManagerT>::value,
                  "ConnectionManagerT not derived from ConnectionManager");
    m_connectionManager = std::make_unique<ConnectionManagerT>(m_reportingHandler);

    LogIfTestOverridesAllowed(m_reportingHandler);
}

template <typename ConnectionManagerT>
std::unique_ptr<VersionEntity> SFSClientImpl<ConnectionManagerT>::GetLatestVersion(const ProductRequest& productRequest,
                                                                                   Connection& connection) const
try
{
    const auto& [product, attributes] = productRequest;
    const std::string url{MakeUrlBuilder().GetLatestVersionUrl(product)};

    LOG_INFO(m_reportingHandler, "Requesting latest version of [%s] from URL [%s]", product.c_str(), url.c_str());

    const json body = {{"TargetingAttributes", attributes}};
    LOG_VERBOSE(m_reportingHandler, "Request body [%s]", body.dump().c_str());

    const std::string postResponse{connection.Post(url, body.dump())};
    const json versionResponse = ParseServerMethodStringToJson(postResponse, "GetLatestVersion", m_reportingHandler);

    auto versionEntity = VersionEntity::FromJson(versionResponse, m_reportingHandler);
    ValidateVersionEntity(*versionEntity, m_nameSpace, product, m_reportingHandler);

    LOG_INFO(m_reportingHandler, "Received a response with version %s", versionEntity->contentId.version.c_str());

    return versionEntity;
}
SFS_CATCH_LOG_RETHROW(m_reportingHandler)

template <typename ConnectionManagerT>
VersionEntities SFSClientImpl<ConnectionManagerT>::GetLatestVersionBatch(
    const std::vector<ProductRequest>& productRequests,
    Connection& connection) const
try
{
    const std::string url{MakeUrlBuilder().GetLatestVersionBatchUrl()};

    LOG_INFO(m_reportingHandler, "Requesting latest version of multiple products from URL [%s]", url.c_str());

    // Creating request body
    std::unordered_set<std::string> requestedProducts;
    json body = json::array();
    for (const auto& [product, attributes] : productRequests)
    {
        LOG_INFO(m_reportingHandler, "Product #%zu: [%s]", body.size() + size_t{1}, product.c_str());
        requestedProducts.insert(product);

        body.push_back({{"TargetingAttributes", attributes}, {"Product", product}});
    }

    LOG_VERBOSE(m_reportingHandler, "Request body [%s]", body.dump().c_str());

    const std::string postResponse{connection.Post(url, body.dump())};

    const json versionResponse =
        ParseServerMethodStringToJson(postResponse, "GetLatestVersionBatch", m_reportingHandler);

    auto entities = ConvertLatestVersionBatchResponseToVersionEntities(versionResponse, m_reportingHandler);
    ValidateBatchVersionEntity(entities, m_nameSpace, requestedProducts, m_reportingHandler);

    return entities;
}
SFS_CATCH_LOG_RETHROW(m_reportingHandler)

template <typename ConnectionManagerT>
std::unique_ptr<VersionEntity> SFSClientImpl<ConnectionManagerT>::GetSpecificVersion(const std::string& product,
                                                                                     const std::string& version,
                                                                                     Connection& connection) const
try
{
    const std::string url{MakeUrlBuilder().GetSpecificVersionUrl(product, version)};

    LOG_INFO(m_reportingHandler,
             "Requesting version [%s] of [%s] from URL [%s]",
             version.c_str(),
             product.c_str(),
             url.c_str());

    const std::string getResponse{connection.Get(url)};

    const json versionResponse = ParseServerMethodStringToJson(getResponse, "GetSpecificVersion", m_reportingHandler);

    auto versionEntity = VersionEntity::FromJson(versionResponse, m_reportingHandler);
    ValidateVersionEntity(*versionEntity, m_nameSpace, product, m_reportingHandler);

    LOG_INFO(m_reportingHandler,
             "Received the expected response with version %s",
             versionEntity->contentId.version.c_str());

    return versionEntity;
}
SFS_CATCH_LOG_RETHROW(m_reportingHandler)

template <typename ConnectionManagerT>
FileEntities SFSClientImpl<ConnectionManagerT>::GetDownloadInfo(const std::string& product,
                                                                const std::string& version,
                                                                Connection& connection) const
try
{
    const std::string url{MakeUrlBuilder().GetDownloadInfoUrl(product, version)};

    LOG_INFO(m_reportingHandler,
             "Requesting download info of version [%s] of [%s] from URL [%s]",
             version.c_str(),
             product.c_str(),
             url.c_str());

    const std::string postResponse{connection.Post(url)};

    const json downloadInfoResponse =
        ParseServerMethodStringToJson(postResponse, "GetDownloadInfo", m_reportingHandler);

    auto files = FileEntity::DownloadInfoResponseToFileEntities(downloadInfoResponse, m_reportingHandler);

    LOG_INFO(m_reportingHandler, "Received a response with %zu files", files.size());

    return files;
}
SFS_CATCH_LOG_RETHROW(m_reportingHandler)

template <typename ConnectionManagerT>
std::vector<Content> SFSClientImpl<ConnectionManagerT>::GetLatestDownloadInfo(const RequestParams& requestParams) const
try
{
    ValidateRequestParams(requestParams, m_reportingHandler);

    const auto connection = MakeConnection(ConnectionConfig(requestParams));

    auto versionEntity = GetLatestVersion(requestParams.productRequests[0], *connection);
    auto contentId = VersionEntity::ToContentId(std::move(*versionEntity), m_reportingHandler);

    const auto& product = requestParams.productRequests[0].product;
    auto fileEntities = GetDownloadInfo(product, contentId->GetVersion(), *connection);
    auto files = GenericFileEntity::FileEntitiesToFileVector(std::move(fileEntities), m_reportingHandler);

    std::unique_ptr<Content> content;
    THROW_IF_FAILED_LOG(Content::Make(std::move(contentId), std::move(files), content), m_reportingHandler);

    std::vector<Content> contents;
    contents.push_back(std::move(*content));

    return contents;
}
SFS_CATCH_LOG_RETHROW(m_reportingHandler)

template <typename ConnectionManagerT>
std::vector<AppContent> SFSClientImpl<ConnectionManagerT>::GetLatestAppDownloadInfo(
    const RequestParams& requestParams) const
try
{
    ValidateRequestParams(requestParams, m_reportingHandler);

    // TODO #150: For now apps are only coming from the "storeapps" instanceId and the service has requested
    // we double check for it. In the future we should remove this check and allow the user to specify any instanceId
    THROW_CODE_IF_LOG(Unexpected,
                      AreNotEqualI(m_instanceId, "storeapps"),
                      m_reportingHandler,
                      "At this moment only the \"storeapps\" instanceId can send app requests");

    const auto connection = MakeConnection(ConnectionConfig(requestParams));

    auto versionEntity = GetLatestVersion(requestParams.productRequests[0], *connection);

    auto appVersionEntity = AppVersionEntity::GetAppVersionEntityPtr(versionEntity, m_reportingHandler);
    auto contentId = AppVersionEntity::ToContentId(std::move(*appVersionEntity), m_reportingHandler);

    LOG_INFO(m_reportingHandler, "Getting download info for main app content");
    const auto& product = requestParams.productRequests[0].product;
    auto fileEntities = GetDownloadInfo(product, contentId->GetVersion(), *connection);
    auto files = AppFileEntity::FileEntitiesToAppFileVector(std::move(fileEntities), m_reportingHandler);

    std::vector<AppPrerequisiteContent> prerequisites;
    for (auto& prereq : appVersionEntity->prerequisites)
    {
        LOG_INFO(m_reportingHandler, "Getting download info for prerequisite [%s]", prereq.contentId.name.c_str());
        auto prereqContentId = GenericVersionEntity::ToContentId(std::move(prereq), m_reportingHandler);

        auto prereqFileEntities =
            GetDownloadInfo(prereqContentId->GetName(), prereqContentId->GetVersion(), *connection);
        auto prereqFiles =
            AppFileEntity::FileEntitiesToAppFileVector(std::move(prereqFileEntities), m_reportingHandler);

        std::unique_ptr<AppPrerequisiteContent> prereqContent;
        THROW_IF_FAILED_LOG(
            AppPrerequisiteContent::Make(std::move(prereqContentId), std::move(prereqFiles), prereqContent),
            m_reportingHandler);

        prerequisites.push_back(std::move(*prereqContent));
    }

    std::unique_ptr<AppContent> content;
    THROW_IF_FAILED_LOG(AppContent::Make(std::move(contentId),
                                         std::move(appVersionEntity->updateId),
                                         std::move(prerequisites),
                                         std::move(files),
                                         content),
                        m_reportingHandler);

    std::vector<AppContent> contents;
    contents.push_back(std::move(*content));

    return contents;
}
SFS_CATCH_LOG_RETHROW(m_reportingHandler)

template <typename ConnectionManagerT>
std::unique_ptr<Connection> SFSClientImpl<ConnectionManagerT>::MakeConnection(const ConnectionConfig& config) const
{
    return m_connectionManager->MakeConnection(config);
}

template <typename ConnectionManagerT>
void SFSClientImpl<ConnectionManagerT>::SetCustomBaseUrl(std::string customBaseUrl)
{
    m_customBaseUrl = std::move(customBaseUrl);
}

template <typename ConnectionManagerT>
SFSUrlBuilder SFSClientImpl<ConnectionManagerT>::MakeUrlBuilder() const
{
    if (auto envVar = test::GetTestOverride(test::TestOverride::BaseUrl))
    {
        return SFSUrlBuilder(SFSCustomUrl(*envVar), m_instanceId, m_nameSpace, m_reportingHandler);
    }
    if (m_customBaseUrl)
    {
        return SFSUrlBuilder(SFSCustomUrl(*m_customBaseUrl), m_instanceId, m_nameSpace, m_reportingHandler);
    }

    return SFSUrlBuilder(m_accountId, m_instanceId, m_nameSpace, m_reportingHandler);
}

template class SFS::details::SFSClientImpl<CurlConnectionManager>;
template class SFS::details::SFSClientImpl<MockConnectionManager>;
