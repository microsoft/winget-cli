// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "../../../util/SFSExceptionMatcher.h"
#include "../../../util/TestHelper.h"
#include "ReportingHandler.h"
#include "entity/VersionEntity.h"
#include "sfsclient/ContentId.h"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#define TEST(...) TEST_CASE("[VersionEntityTests] " __VA_ARGS__)

using namespace SFS;
using namespace SFS::details;
using namespace SFS::test;
using json = nlohmann::json;

// GenericVersionEntity constants
const std::string c_ns = "namespace";
const std::string c_name = "name";
const std::string c_version = "version";

// AppVersionEntity constants
const std::string c_updateId = "updateId";

TEST("Testing VersionEntity::FromJson()")
{
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);

    SECTION("Generic Version Entity")
    {
        std::unique_ptr<VersionEntity> entity;
        SECTION("Correct")
        {
            const json versionEntity = {{"ContentId", {{"Namespace", c_ns}, {"Name", c_name}, {"Version", c_version}}}};

            REQUIRE_NOTHROW(entity = VersionEntity::FromJson(versionEntity, handler));
            REQUIRE(entity != nullptr);
            REQUIRE(entity->GetContentType() == ContentType::Generic);
            REQUIRE(entity->contentId.nameSpace == c_ns);
            REQUIRE(entity->contentId.name == c_name);
            REQUIRE(entity->contentId.version == c_version);
        }

        SECTION("Missing fields")
        {
            SECTION("Missing ContentId")
            {
                const json versionEntity = {{"Namespace", c_ns}, {"Name", c_name}, {"Version", c_version}};
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing ContentId in response");
            }

            SECTION("Missing ContentId.Namespace")
            {
                const json versionEntity = {{"ContentId", {{"Name", c_name}, {"Version", c_version}}}};
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing ContentId.Namespace in response");
            }

            SECTION("Missing ContentId.Name")
            {
                const json versionEntity = {{"ContentId", {{"Namespace", c_ns}, {"Version", c_version}}}};
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing ContentId.Name in response");
            }

            SECTION("Missing ContentId.Version")
            {
                const json versionEntity = {{"ContentId", {{"Namespace", c_ns}, {"Name", c_name}}}};
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing ContentId.Version in response");
            }
        }

        SECTION("Wrong types")
        {
            SECTION("ContentId not an object")
            {
                const json versionEntity = json::array({{"Namespace", 1}, {"Name", c_name}, {"Version", c_version}});
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "Response is not a JSON object");
            }

            SECTION("ContentId.Namespace")
            {
                const json versionEntity = {
                    {"ContentId", {{"Namespace", 1}, {"Name", c_name}, {"Version", c_version}}}};
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "ContentId.Namespace is not a string");
            }

            SECTION("ContentId.Name")
            {
                const json versionEntity = {{"ContentId", {{"Namespace", c_ns}, {"Name", 1}, {"Version", c_version}}}};
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "ContentId.Name is not a string");
            }

            SECTION("ContentId.Version")
            {
                const json versionEntity = {{"ContentId", {{"Namespace", c_ns}, {"Name", c_name}, {"Version", 1}}}};
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "ContentId.Version is not a string");
            }
        }
    }

    SECTION("App Version Entity")
    {
        std::unique_ptr<VersionEntity> entity;
        const json contentId = {{"Namespace", c_ns}, {"Name", c_name}, {"Version", c_version}};

        SECTION("Correct")
        {
            const json versionEntity = {{"ContentId", contentId},
                                        {"UpdateId", c_updateId},
                                        {"Prerequisites", json::array({contentId})}};

            REQUIRE_NOTHROW(entity = VersionEntity::FromJson(versionEntity, handler));
            REQUIRE(entity != nullptr);
            REQUIRE(entity->GetContentType() == ContentType::App);
            REQUIRE(entity->contentId.nameSpace == c_ns);
            REQUIRE(entity->contentId.name == c_name);
            REQUIRE(entity->contentId.version == c_version);

            AppVersionEntity* appEntity = dynamic_cast<AppVersionEntity*>(entity.get());
            REQUIRE(appEntity->updateId == c_updateId);
            REQUIRE(appEntity->prerequisites.size() == 1);
            REQUIRE(appEntity->prerequisites[0].contentId.nameSpace == c_ns);
            REQUIRE(appEntity->prerequisites[0].contentId.name == c_name);
            REQUIRE(appEntity->prerequisites[0].contentId.version == c_version);
        }

        SECTION("Missing fields")
        {
            SECTION("Missing Prerequisites")
            {
                const json versionEntity = {{"ContentId", contentId}, {"UpdateId", c_updateId}};
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing Prerequisites in response");
            }

            SECTION("Missing Prerequisite.Namespace")
            {
                const json wrongPrerequisite = {{"Name", c_name}, {"Version", c_version}};
                const json versionEntity = {{"ContentId", contentId},
                                            {"UpdateId", c_updateId},
                                            {"Prerequisites", json::array({wrongPrerequisite})}};
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing Prerequisite.Namespace in response");
            }

            SECTION("Missing Prerequisite.Name")
            {
                const json wrongPrerequisite = {{"Namespace", c_ns}, {"Version", c_version}};
                const json versionEntity = {{"ContentId", contentId},
                                            {"UpdateId", c_updateId},
                                            {"Prerequisites", json::array({wrongPrerequisite})}};
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing Prerequisite.Name in response");
            }

            SECTION("Missing Prerequisite.Version")
            {
                const json wrongPrerequisite = {{"Namespace", c_ns}, {"Name", c_name}};
                const json versionEntity = {{"ContentId", contentId},
                                            {"UpdateId", c_updateId},
                                            {"Prerequisites", json::array({wrongPrerequisite})}};
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing Prerequisite.Version in response");
            }
        }

        SECTION("Wrong types")
        {
            SECTION("UpdateId")
            {
                const json versionEntity = {{"ContentId", contentId},
                                            {"UpdateId", 1},
                                            {"Prerequisites", json::array({contentId})}};
                REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                        ServiceInvalidResponse,
                                        "UpdateId is not a string");
            }

            SECTION("Prerequisites")
            {
                SECTION("Not an array")
                {
                    const json versionEntity = {{"ContentId", contentId},
                                                {"UpdateId", c_updateId},
                                                {"Prerequisites", contentId}};
                    REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                            ServiceInvalidResponse,
                                            "Prerequisites is not an array");
                }

                SECTION("Element is not an object")
                {
                    const json versionEntity = {{"ContentId", contentId},
                                                {"UpdateId", c_updateId},
                                                {"Prerequisites", json::array({1})}};
                    REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                            ServiceInvalidResponse,
                                            "Prerequisite element is not a JSON object");
                }

                SECTION("Prerequisite.Namespace")
                {
                    const json wrongPrerequisite = {{"Namespace", 1}, {"Name", c_name}, {"Version", c_version}};
                    const json versionEntity = {{"ContentId", contentId},
                                                {"UpdateId", c_updateId},
                                                {"Prerequisites", json::array({wrongPrerequisite})}};
                    REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                            ServiceInvalidResponse,
                                            "Prerequisite.Namespace is not a string");
                }

                SECTION("Prerequisite.Name")
                {
                    const json wrongPrerequisite = {{"Namespace", c_ns}, {"Name", 1}, {"Version", c_version}};
                    const json versionEntity = {{"ContentId", contentId},
                                                {"UpdateId", c_updateId},
                                                {"Prerequisites", json::array({wrongPrerequisite})}};
                    REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                            ServiceInvalidResponse,
                                            "Prerequisite.Name is not a string");
                }

                SECTION("Prerequisite.Version")
                {
                    const json wrongPrerequisite = {{"Namespace", c_ns}, {"Name", c_name}, {"Version", 1}};
                    const json versionEntity = {{"ContentId", contentId},
                                                {"UpdateId", c_updateId},
                                                {"Prerequisites", json::array({wrongPrerequisite})}};
                    REQUIRE_THROWS_CODE_MSG(VersionEntity::FromJson(versionEntity, handler),
                                            ServiceInvalidResponse,
                                            "Prerequisite.Version is not a string");
                }
            }
        }
    }
}

TEST("Testing VersionEntity conversions")
{
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);

    auto CheckContentId = [&](const ContentId& contentId) {
        REQUIRE(contentId.GetNameSpace() == c_ns);
        REQUIRE(contentId.GetName() == c_name);
        REQUIRE(contentId.GetVersion() == c_version);
    };

    SECTION("VersionEntity::ToContentId()")
    {
        SECTION("Success with GenericVersionEntity")
        {
            std::unique_ptr<VersionEntity> entity = std::make_unique<GenericVersionEntity>();
            entity->contentId.nameSpace = c_ns;
            entity->contentId.name = c_name;
            entity->contentId.version = c_version;

            auto contentId = VersionEntity::ToContentId(std::move(*entity), handler);
            CheckContentId(*contentId);
        }

        SECTION("Success with AppVersionEntity")
        {
            std::unique_ptr<VersionEntity> entity = std::make_unique<AppVersionEntity>();
            auto appEntity = AppVersionEntity::GetAppVersionEntityPtr(entity, handler);
            appEntity->contentId.nameSpace = c_ns;
            appEntity->contentId.name = c_name;
            appEntity->contentId.version = c_version;
            appEntity->updateId = c_updateId;

            std::unique_ptr<GenericVersionEntity> prereqEntity = std::make_unique<GenericVersionEntity>();
            prereqEntity->contentId.nameSpace = c_ns;
            prereqEntity->contentId.name = c_name;
            prereqEntity->contentId.version = c_version;

            appEntity->prerequisites.push_back(std::move(*prereqEntity));

            auto contentId = VersionEntity::ToContentId(std::move(*entity), handler);
            CheckContentId(*contentId);
        }
    }
}
