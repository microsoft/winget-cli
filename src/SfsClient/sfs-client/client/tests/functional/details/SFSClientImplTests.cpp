// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "../../mock/MockWebServer.h"
#include "../../util/SFSExceptionMatcher.h"
#include "../../util/TestHelper.h"
#include "SFSClientImpl.h"
#include "connection/Connection.h"
#include "connection/CurlConnectionManager.h"
#include "connection/HttpHeader.h"

#include <catch2/catch_test_macros.hpp>

#include <set>

#define TEST(...) TEST_CASE("[Functional][SFSClientImplTests] " __VA_ARGS__)

using namespace SFS;
using namespace SFS::details;
using namespace SFS::test;

namespace
{
void CheckProduct(const VersionEntity& entity, std::string_view ns, std::string_view name, std::string_view version)
{
    REQUIRE(entity.GetContentType() == ContentType::Generic);
    REQUIRE(entity.contentId.nameSpace == ns);
    REQUIRE(entity.contentId.name == name);
    REQUIRE(entity.contentId.version == version);
}

void CheckAppProduct(const VersionEntity& entity, std::string_view ns, std::string_view name, std::string_view version)
{
    REQUIRE(entity.GetContentType() == ContentType::App);
    auto appEntity = dynamic_cast<const AppVersionEntity&>(entity);
    REQUIRE(appEntity.contentId.nameSpace == ns);
    REQUIRE(appEntity.contentId.name == name);
    REQUIRE(appEntity.contentId.version == version);
    REQUIRE_FALSE(appEntity.updateId.empty());
    REQUIRE(appEntity.prerequisites.empty());
}

void CheckProducts(const VersionEntities& entities,
                   std::string_view ns,
                   const std::set<std::pair<std::string, std::string>>& nameVersionPairs)
{
    // json arrays don't guarantee order, and they are the underlying structure, so we need to check using sets
    std::set<std::pair<std::string, std::string>> uniqueNameVersionPairs;
    for (const auto& entity : entities)
    {
        REQUIRE(entity->contentId.nameSpace == ns);
        uniqueNameVersionPairs.emplace(entity->contentId.name, entity->contentId.version);
    }

    for (const auto& nameVersionPair : nameVersionPairs)
    {
        REQUIRE(uniqueNameVersionPairs.count(nameVersionPair));
    }
}

void CheckDownloadInfo(const FileEntities& files, const std::string& name)
{
    REQUIRE(files.size() == 2);
    REQUIRE(files[0]->GetContentType() == ContentType::Generic);
    REQUIRE(files[0]->fileId == (name + ".json"));
    REQUIRE(files[0]->url == ("http://localhost/1.json"));
    REQUIRE(files[1]->GetContentType() == ContentType::Generic);
    REQUIRE(files[1]->fileId == (name + ".bin"));
    REQUIRE(files[1]->url == ("http://localhost/2.bin"));
}

void CheckAppDownloadInfo(const FileEntities& files, const std::string& name)
{
    REQUIRE(files.size() == 2);
    REQUIRE(files[0]->GetContentType() == ContentType::App);
    REQUIRE(files[0]->fileId == (name + ".json"));
    REQUIRE(files[0]->url == ("http://localhost/1.json"));
    REQUIRE(files[1]->GetContentType() == ContentType::App);
    REQUIRE(files[1]->fileId == (name + ".bin"));
    REQUIRE(files[1]->url == ("http://localhost/2.bin"));
}
} // namespace

TEST("Testing class SFSClientImpl()")
{
    test::MockWebServer server;
    const std::string ns = "testNameSpace";
    SFSClientImpl<CurlConnectionManager> sfsClient({"testAccountId", "testInstanceId", ns, LogCallbackToTest});
    sfsClient.SetCustomBaseUrl(server.GetBaseUrl());

    const std::string cv = "aaaaaaaaaaaaaaaa.1";
    ConnectionConfig config;
    config.baseCV = cv;
    auto connection = sfsClient.MakeConnection(config);
    server.RegisterExpectedRequestHeader(HttpHeader::UserAgent, GetUserAgentValue());

    SECTION("Generic products")
    {
        server.RegisterProduct("productName", "0.0.0.2");
        server.RegisterProduct("productName", "0.0.0.1");

        SECTION("Testing SFSClientImpl::GetLatestVersion()")
        {
            server.RegisterExpectedRequestHeader(HttpHeader::ContentType, "application/json");
            std::unique_ptr<VersionEntity> entity;

            SECTION("No attributes")
            {
                REQUIRE_NOTHROW(entity = sfsClient.GetLatestVersion({"productName", {}}, *connection));
                REQUIRE(entity);
                CheckProduct(*entity, ns, "productName", "0.0.0.2");
            }

            SECTION("With attributes")
            {
                const TargetingAttributes attributes{{"attr1", "value"}};
                REQUIRE_NOTHROW(entity = sfsClient.GetLatestVersion({"productName", attributes}, *connection));
                REQUIRE(entity);
                CheckProduct(*entity, ns, "productName", "0.0.0.2");
            }

            SECTION("Wrong product name")
            {
                REQUIRE_THROWS_CODE(entity = sfsClient.GetLatestVersion({"badName", {}}, *connection), HttpNotFound);
                REQUIRE(!entity);

                const TargetingAttributes attributes{{"attr1", "value"}};
                REQUIRE_THROWS_CODE(entity = sfsClient.GetLatestVersion({"badName", attributes}, *connection),
                                    HttpNotFound);
                REQUIRE(!entity);
            }
        }

        SECTION("Testing SFSClientImpl::GetLatestVersionBatch()")
        {
            server.RegisterExpectedRequestHeader(HttpHeader::ContentType, "application/json");
            VersionEntities entities;

            SECTION("No attributes")
            {
                REQUIRE_NOTHROW(entities = sfsClient.GetLatestVersionBatch({{"productName", {}}}, *connection));
                REQUIRE(!entities.empty());
                CheckProduct(*entities[0], ns, "productName", "0.0.0.2");
            }

            SECTION("With attributes")
            {
                const TargetingAttributes attributes{{"attr1", "value"}};
                REQUIRE_NOTHROW(entities = sfsClient.GetLatestVersionBatch({{"productName", attributes}}, *connection));
                REQUIRE(!entities.empty());
                CheckProduct(*entities[0], ns, "productName", "0.0.0.2");
            }

            SECTION("Wrong product name")
            {
                REQUIRE_THROWS_CODE(entities = sfsClient.GetLatestVersionBatch({{"badName", {}}}, *connection),
                                    HttpNotFound);
                REQUIRE(entities.empty());

                const TargetingAttributes attributes{{"attr1", "value"}};
                REQUIRE_THROWS_CODE(entities = sfsClient.GetLatestVersionBatch({{"badName", attributes}}, *connection),
                                    HttpNotFound);
                REQUIRE(entities.empty());
            }

            SECTION("Multiple unique products")
            {
                server.RegisterProduct("productName2", "0.0.0.3");

                REQUIRE_NOTHROW(entities = sfsClient.GetLatestVersionBatch({{"productName", {}}, {"productName2", {}}},
                                                                           *connection));
                REQUIRE(entities.size() == 2);
                CheckProducts(entities, ns, {{"productName", "0.0.0.2"}, {"productName2", "0.0.0.3"}});

                server.RegisterProduct("productName3", "0.0.0.4");

                REQUIRE_NOTHROW(entities = sfsClient.GetLatestVersionBatch(
                                    {{"productName", {}}, {"productName2", {}}, {"productName3", {}}},
                                    *connection));
                REQUIRE(entities.size() == 3);
                CheckProducts(entities,
                              ns,
                              {{"productName", "0.0.0.2"}, {"productName2", "0.0.0.3"}, {"productName3", "0.0.0.4"}});
            }

            SECTION("Multiple repeated products")
            {
                REQUIRE_NOTHROW(entities = sfsClient.GetLatestVersionBatch({{"productName", {}}, {"productName", {}}},
                                                                           *connection));
                REQUIRE(entities.size() == 1);
                CheckProduct(*entities[0], ns, "productName", "0.0.0.2");

                server.RegisterProduct("productName2", "0.0.0.3");

                REQUIRE_NOTHROW(
                    entities = sfsClient.GetLatestVersionBatch(
                        {{"productName", {}}, {"productName", {}}, {"productName2", {}}, {"productName2", {}}},
                        *connection));
                REQUIRE(entities.size() == 2);
                CheckProducts(entities, ns, {{"productName", "0.0.0.2"}, {"productName2", "0.0.0.3"}});
            }

            SECTION("Multiple wrong products returns 404")
            {
                REQUIRE_THROWS_CODE(
                    entities = sfsClient.GetLatestVersionBatch({{"badName", {}}, {"badName2", {}}}, *connection),
                    HttpNotFound);
                REQUIRE(entities.empty());
            }

            SECTION("Multiple products, one wrong returns 200")
            {
                REQUIRE_NOTHROW(
                    entities = sfsClient.GetLatestVersionBatch({{"productName", {}}, {"badName", {}}}, *connection));
                REQUIRE(entities.size() == 1);
                CheckProduct(*entities[0], ns, "productName", "0.0.0.2");
            }
        }

        SECTION("Testing SFSClientImpl::GetSpecificVersion()")
        {
            std::unique_ptr<VersionEntity> entity;
            SECTION("Getting 0.0.0.1")
            {
                REQUIRE_NOTHROW(entity = sfsClient.GetSpecificVersion("productName", "0.0.0.1", *connection));
                REQUIRE(entity);
                CheckProduct(*entity, ns, "productName", "0.0.0.1");
            }

            SECTION("Getting 0.0.0.2")
            {
                REQUIRE_NOTHROW(entity = sfsClient.GetSpecificVersion("productName", "0.0.0.2", *connection));
                REQUIRE(entity);
                CheckProduct(*entity, ns, "productName", "0.0.0.2");
            }

            SECTION("Wrong product name")
            {
                REQUIRE_THROWS_CODE(entity = sfsClient.GetSpecificVersion("badName", "0.0.0.2", *connection),
                                    HttpNotFound);
                REQUIRE(!entity);
            }

            SECTION("Wrong version")
            {
                REQUIRE_THROWS_CODE(entity = sfsClient.GetSpecificVersion("productName", "0.0.0.3", *connection),
                                    HttpNotFound);
                REQUIRE(!entity);
            }
        }

        SECTION("Testing SFSClientImpl::GetDownloadInfo()")
        {
            server.RegisterExpectedRequestHeader(HttpHeader::ContentType, "application/json");
            FileEntities files;

            SECTION("Getting 0.0.0.1")
            {
                REQUIRE_NOTHROW(files = sfsClient.GetDownloadInfo("productName", "0.0.0.1", *connection));
                REQUIRE(!files.empty());
                CheckDownloadInfo(files, "productName");
            }

            SECTION("Getting 0.0.0.2")
            {
                REQUIRE_NOTHROW(files = sfsClient.GetDownloadInfo("productName", "0.0.0.2", *connection));
                REQUIRE(!files.empty());
                CheckDownloadInfo(files, "productName");
            }

            SECTION("Wrong product name")
            {
                REQUIRE_THROWS_CODE(files = sfsClient.GetDownloadInfo("badName", "0.0.0.2", *connection), HttpNotFound);
                REQUIRE(files.empty());
            }

            SECTION("Wrong version")
            {
                REQUIRE_THROWS_CODE(files = sfsClient.GetDownloadInfo("productName", "0.0.0.3", *connection),
                                    HttpNotFound);
                REQUIRE(files.empty());
            }
        }
    }

    SECTION("App products")
    {
        server.RegisterAppProduct("productName", "0.0.0.2", {});
        server.RegisterAppProduct("productName", "0.0.0.1", {});

        SECTION("Testing SFSClientImpl::GetLatestVersion()")
        {
            server.RegisterExpectedRequestHeader(HttpHeader::ContentType, "application/json");
            std::unique_ptr<VersionEntity> entity;

            SECTION("No attributes")
            {
                REQUIRE_NOTHROW(entity = sfsClient.GetLatestVersion({"productName", {}}, *connection));
                REQUIRE(entity);
                CheckAppProduct(*entity, ns, "productName", "0.0.0.2");
            }

            SECTION("With attributes")
            {
                const TargetingAttributes attributes{{"attr1", "value"}};
                REQUIRE_NOTHROW(entity = sfsClient.GetLatestVersion({"productName", attributes}, *connection));
                REQUIRE(entity);
                CheckAppProduct(*entity, ns, "productName", "0.0.0.2");
            }

            SECTION("Wrong product name")
            {
                REQUIRE_THROWS_CODE(entity = sfsClient.GetLatestVersion({"badName", {}}, *connection), HttpNotFound);
                REQUIRE(!entity);

                const TargetingAttributes attributes{{"attr1", "value"}};
                REQUIRE_THROWS_CODE(entity = sfsClient.GetLatestVersion({"badName", attributes}, *connection),
                                    HttpNotFound);
                REQUIRE(!entity);
            }
        }

        SECTION("Testing SFSClientImpl::GetDownloadInfo()")
        {
            server.RegisterExpectedRequestHeader(HttpHeader::ContentType, "application/json");
            FileEntities files;

            SECTION("Getting 0.0.0.1")
            {
                REQUIRE_NOTHROW(files = sfsClient.GetDownloadInfo("productName", "0.0.0.1", *connection));
                REQUIRE(!files.empty());
                CheckAppDownloadInfo(files, "productName");
            }

            SECTION("Getting 0.0.0.2")
            {
                REQUIRE_NOTHROW(files = sfsClient.GetDownloadInfo("productName", "0.0.0.2", *connection));
                REQUIRE(!files.empty());
                CheckAppDownloadInfo(files, "productName");
            }

            SECTION("Wrong product name")
            {
                REQUIRE_THROWS_CODE(files = sfsClient.GetDownloadInfo("badName", "0.0.0.2", *connection), HttpNotFound);
                REQUIRE(files.empty());
            }

            SECTION("Wrong version")
            {
                REQUIRE_THROWS_CODE(files = sfsClient.GetDownloadInfo("productName", "0.0.0.3", *connection),
                                    HttpNotFound);
                REQUIRE(files.empty());
            }
        }
    }

    REQUIRE(server.Stop() == Result::Success);
}
