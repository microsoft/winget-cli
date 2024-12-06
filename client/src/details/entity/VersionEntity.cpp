// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "VersionEntity.h"

#include "../ErrorHandling.h"
#include "../ReportingHandler.h"
#include "ContentId.h"

#include <nlohmann/json.hpp>

#define THROW_INVALID_RESPONSE_IF_NOT(condition, message, handler)                                                     \
    THROW_CODE_IF_NOT_LOG(ServiceInvalidResponse, condition, handler, message)

using namespace SFS;
using namespace SFS::details;
using json = nlohmann::json;

namespace
{
void ValidateContentType(const VersionEntity& entity, ContentType expectedType, const ReportingHandler& handler)
{
    THROW_CODE_IF_LOG(Result::ServiceUnexpectedContentType,
                      entity.GetContentType() != expectedType,
                      handler,
                      "The service returned entity \"" + entity.contentId.name + "\" with content type [" +
                          ToString(entity.GetContentType()) + "] while the expected type was [" +
                          ToString(expectedType) + "]");
}
} // namespace

std::unique_ptr<VersionEntity> VersionEntity::FromJson(const nlohmann::json& data, const ReportingHandler& handler)
{
    // Expected format for a generic version entity:
    // {
    //   "ContentId": {
    //     "Namespace": <ns>,
    //     "Name": <name>,
    //     "Version": <version>
    //   }
    // }
    //
    // Expected extra elements for an app version entity:
    // {
    //   ...
    //   "UpdateId": "<id>",
    //   "Prerequisites": [
    //     {
    //       "Namespace": "<ns>",
    //       "Name": "<name>",
    //       "Version": "<version>"
    //     }
    //   ]
    // }

    THROW_INVALID_RESPONSE_IF_NOT(data.is_object(), "Response is not a JSON object", handler);

    std::unique_ptr<VersionEntity> tmp;
    const bool isAppEntity = data.contains("UpdateId");
    if (isAppEntity)
    {
        tmp = std::make_unique<AppVersionEntity>();
    }
    else
    {
        tmp = std::make_unique<GenericVersionEntity>();
    }

    THROW_INVALID_RESPONSE_IF_NOT(data.contains("ContentId"), "Missing ContentId in response", handler);

    const auto& contentId = data["ContentId"];
    THROW_INVALID_RESPONSE_IF_NOT(contentId.is_object(), "ContentId is not a JSON object", handler);

    THROW_INVALID_RESPONSE_IF_NOT(contentId.contains("Namespace"), "Missing ContentId.Namespace in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(contentId["Namespace"].is_string(), "ContentId.Namespace is not a string", handler);
    tmp->contentId.nameSpace = contentId["Namespace"];

    THROW_INVALID_RESPONSE_IF_NOT(contentId.contains("Name"), "Missing ContentId.Name in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(contentId["Name"].is_string(), "ContentId.Name is not a string", handler);
    tmp->contentId.name = contentId["Name"];

    THROW_INVALID_RESPONSE_IF_NOT(contentId.contains("Version"), "Missing ContentId.Version in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(contentId["Version"].is_string(), "ContentId.Version is not a string", handler);
    tmp->contentId.version = contentId["Version"];

    if (isAppEntity)
    {
        auto appEntity = dynamic_cast<AppVersionEntity*>(tmp.get());

        THROW_INVALID_RESPONSE_IF_NOT(data["UpdateId"].is_string(), "UpdateId is not a string", handler);
        appEntity->updateId = data["UpdateId"];

        THROW_INVALID_RESPONSE_IF_NOT(data.contains("Prerequisites"), "Missing Prerequisites in response", handler);
        THROW_INVALID_RESPONSE_IF_NOT(data["Prerequisites"].is_array(), "Prerequisites is not an array", handler);

        for (const auto& prereq : data["Prerequisites"])
        {
            THROW_INVALID_RESPONSE_IF_NOT(prereq.is_object(), "Prerequisite element is not a JSON object", handler);

            GenericVersionEntity prereqEntity;
            THROW_INVALID_RESPONSE_IF_NOT(prereq.contains("Namespace"),
                                          "Missing Prerequisite.Namespace in response",
                                          handler);
            THROW_INVALID_RESPONSE_IF_NOT(prereq["Namespace"].is_string(),
                                          "Prerequisite.Namespace is not a string",
                                          handler);
            prereqEntity.contentId.nameSpace = prereq["Namespace"];

            THROW_INVALID_RESPONSE_IF_NOT(prereq.contains("Name"), "Missing Prerequisite.Name in response", handler);
            THROW_INVALID_RESPONSE_IF_NOT(prereq["Name"].is_string(), "Prerequisite.Name is not a string", handler);
            prereqEntity.contentId.name = prereq["Name"];

            THROW_INVALID_RESPONSE_IF_NOT(prereq.contains("Version"),
                                          "Missing Prerequisite.Version in response",
                                          handler);
            THROW_INVALID_RESPONSE_IF_NOT(prereq["Version"].is_string(),
                                          "Prerequisite.Version is not a string",
                                          handler);
            prereqEntity.contentId.version = prereq["Version"];

            appEntity->prerequisites.push_back(std::move(prereqEntity));
        }
    }
    return tmp;
}

std::unique_ptr<ContentId> VersionEntity::ToContentId(VersionEntity&& entity, const ReportingHandler& handler)
{
    std::unique_ptr<ContentId> tmp;
    THROW_IF_FAILED_LOG(ContentId::Make(std::move(entity.contentId.nameSpace),
                                        std::move(entity.contentId.name),
                                        std::move(entity.contentId.version),
                                        tmp),
                        handler);
    return tmp;
}

ContentType GenericVersionEntity::GetContentType() const
{
    return ContentType::Generic;
}

ContentType AppVersionEntity::GetContentType() const
{
    return ContentType::App;
}

AppVersionEntity* AppVersionEntity::GetAppVersionEntityPtr(std::unique_ptr<VersionEntity>& versionEntity,
                                                           const ReportingHandler& handler)
{
    ValidateContentType(*versionEntity, ContentType::App, handler);
    return dynamic_cast<AppVersionEntity*>(versionEntity.get());
}
