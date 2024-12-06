// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "FileEntity.h"

#include "../ErrorHandling.h"
#include "../ReportingHandler.h"
#include "../Util.h"
#include "AppFile.h"
#include "File.h"

#include <nlohmann/json.hpp>

#define THROW_INVALID_RESPONSE_IF_NOT(condition, message, handler)                                                     \
    THROW_CODE_IF_NOT_LOG(ServiceInvalidResponse, condition, handler, message)

using namespace SFS;
using namespace SFS::details;
using namespace SFS::details::util;
using json = nlohmann::json;

namespace
{
HashType HashTypeFromString(const std::string& hashType, const ReportingHandler& handler)
{
    if (AreEqualI(hashType, "Sha1"))
    {
        return HashType::Sha1;
    }
    else if (AreEqualI(hashType, "Sha256"))
    {
        return HashType::Sha256;
    }
    else
    {
        THROW_LOG(Result(Result::Unexpected, "Unknown hash type: " + hashType), handler);
        return HashType::Sha1; // Unreachable code, but the compiler doesn't know that.
    }
}

Architecture ArchitectureFromString(const std::string& arch, const ReportingHandler& handler)
{
    if (AreEqualI(arch, "None"))
    {
        return Architecture::None;
    }
    else if (AreEqualI(arch, "x86"))
    {
        return Architecture::x86;
    }
    else if (AreEqualI(arch, "amd64"))
    {
        return Architecture::Amd64;
    }
    else if (AreEqualI(arch, "arm"))
    {
        return Architecture::Arm;
    }
    else if (AreEqualI(arch, "arm64"))
    {
        return Architecture::Arm64;
    }
    else
    {
        THROW_LOG(Result(Result::Unexpected, "Unknown architecture: " + arch), handler);
        return Architecture::None; // Unreachable code, but the compiler doesn't know that.
    }
}

void ValidateContentType(const FileEntity& entity, ContentType expectedType, const ReportingHandler& handler)
{
    THROW_CODE_IF_LOG(Result::ServiceUnexpectedContentType,
                      entity.GetContentType() != expectedType,
                      handler,
                      "The service returned file \"" + entity.fileId + "\" with content type [" +
                          ToString(entity.GetContentType()) + "] while the expected type was [" +
                          ToString(expectedType) + "]");
}
} // namespace

std::unique_ptr<FileEntity> FileEntity::FromJson(const nlohmann::json& file, const ReportingHandler& handler)
{
    // Expected format for a generic file entity:
    // {
    //   "FileId": <fileid>,
    //   "Url": <url>,
    //   "SizeInBytes": <size>,
    //   "Hashes": {
    //     "Sha1": <sha1>,
    //     "Sha256": <sha2>
    //   },
    //   "DeliveryOptimization": {} // ignored, not used by the client.
    // }
    //
    // Expected extra elements for an app version entity:
    // {
    //   ...
    //   "ApplicabilityDetails": {
    //     "Architectures": [
    //       "<arch>"
    //     ],
    //     "PlatformApplicabilityForPackage": [
    //       "<app>"
    //     ]
    //   },
    //   "FileMoniker": "<moniker>",
    // }

    THROW_INVALID_RESPONSE_IF_NOT(file.is_object(), "File is not a JSON object", handler);

    std::unique_ptr<FileEntity> tmp;
    const bool isAppEntity = file.contains("FileMoniker");
    if (isAppEntity)
    {
        tmp = std::make_unique<AppFileEntity>();
    }
    else
    {
        tmp = std::make_unique<GenericFileEntity>();
    }

    THROW_INVALID_RESPONSE_IF_NOT(file.contains("FileId"), "Missing File.FileId in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(file["FileId"].is_string(), "File.FileId is not a string", handler);
    tmp->fileId = file["FileId"];

    THROW_INVALID_RESPONSE_IF_NOT(file.contains("Url"), "Missing File.Url in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(file["Url"].is_string(), "File.Url is not a string", handler);
    tmp->url = file["Url"];

    THROW_INVALID_RESPONSE_IF_NOT(file.contains("SizeInBytes"), "Missing File.SizeInBytes in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(file["SizeInBytes"].is_number_unsigned(),
                                  "File.SizeInBytes is not an unsigned number",
                                  handler);
    tmp->sizeInBytes = file["SizeInBytes"];

    THROW_INVALID_RESPONSE_IF_NOT(file.contains("Hashes"), "Missing File.Hashes in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(file["Hashes"].is_object(), "File.Hashes is not an object", handler);

    for (const auto& [hashType, hashValue] : file["Hashes"].items())
    {
        THROW_INVALID_RESPONSE_IF_NOT(hashValue.is_string(), "File.Hashes object value is not a string", handler);
        tmp->hashes[hashType] = hashValue;
    }

    if (isAppEntity)
    {
        auto appEntity = dynamic_cast<AppFileEntity*>(tmp.get());

        THROW_INVALID_RESPONSE_IF_NOT(file["FileMoniker"].is_string(), "File.FileMoniker is not a string", handler);
        appEntity->fileMoniker = file["FileMoniker"];

        THROW_INVALID_RESPONSE_IF_NOT(file.contains("ApplicabilityDetails"),
                                      "Missing File.ApplicabilityDetails in response",
                                      handler);

        const auto& details = file["ApplicabilityDetails"];
        THROW_INVALID_RESPONSE_IF_NOT(details.is_object(), "File.ApplicabilityDetails is not an object", handler);

        THROW_INVALID_RESPONSE_IF_NOT(details.contains("Architectures"),
                                      "Missing File.ApplicabilityDetails.Architectures in response",
                                      handler);
        THROW_INVALID_RESPONSE_IF_NOT(details["Architectures"].is_array(),
                                      "File.ApplicabilityDetails.Architectures is not an array",
                                      handler);
        for (const auto& arch : details["Architectures"])
        {
            THROW_INVALID_RESPONSE_IF_NOT(arch.is_string(),
                                          "File.ApplicabilityDetails.Architectures array value is not a string",
                                          handler);
        }
        appEntity->applicabilityDetails.architectures = details["Architectures"];

        THROW_INVALID_RESPONSE_IF_NOT(details.contains("PlatformApplicabilityForPackage"),
                                      "Missing File.ApplicabilityDetails.PlatformApplicabilityForPackage in response",
                                      handler);
        THROW_INVALID_RESPONSE_IF_NOT(details["PlatformApplicabilityForPackage"].is_array(),
                                      "File.ApplicabilityDetails.PlatformApplicabilityForPackage is not an array",
                                      handler);
        for (const auto& app : details["PlatformApplicabilityForPackage"])
        {
            THROW_INVALID_RESPONSE_IF_NOT(
                app.is_string(),
                "File.ApplicabilityDetails.PlatformApplicabilityForPackage array value is not a string",
                handler);
        }
        appEntity->applicabilityDetails.platformApplicabilityForPackage = details["PlatformApplicabilityForPackage"];
    }

    return tmp;
}

FileEntities FileEntity::DownloadInfoResponseToFileEntities(const nlohmann::json& data, const ReportingHandler& handler)
{
    // Expected format is an array of FileEntity
    THROW_CODE_IF_NOT_LOG(ServiceInvalidResponse, data.is_array(), handler, "Response is not a JSON array");

    FileEntities tmp;
    for (const auto& fileData : data)
    {
        THROW_CODE_IF_NOT_LOG(ServiceInvalidResponse,
                              fileData.is_object(),
                              handler,
                              "Array element is not a JSON object");
        tmp.push_back(std::move(FileEntity::FromJson(fileData, handler)));
    }

    return tmp;
}

ContentType GenericFileEntity::GetContentType() const
{
    return ContentType::Generic;
}

std::unique_ptr<File> GenericFileEntity::ToFile(FileEntity&& entity, const ReportingHandler& handler)
{
    ValidateContentType(entity, ContentType::Generic, handler);

    std::unordered_map<HashType, std::string> hashes;
    for (auto& [hashType, hashValue] : entity.hashes)
    {
        hashes[HashTypeFromString(hashType, handler)] = std::move(hashValue);
    }

    std::unique_ptr<File> tmp;
    THROW_IF_FAILED_LOG(
        File::Make(std::move(entity.fileId), std::move(entity.url), entity.sizeInBytes, std::move(hashes), tmp),
        handler);
    return tmp;
}

std::vector<File> GenericFileEntity::FileEntitiesToFileVector(FileEntities&& entities, const ReportingHandler& handler)
{
    std::vector<File> tmp;
    for (auto& entity : entities)
    {
        tmp.push_back(std::move(*GenericFileEntity::ToFile(std::move(*entity), handler)));
    }

    return tmp;
}

ContentType AppFileEntity::GetContentType() const
{
    return ContentType::App;
}

std::unique_ptr<AppFile> AppFileEntity::ToAppFile(FileEntity&& entity, const ReportingHandler& handler)
{
    ValidateContentType(entity, ContentType::App, handler);

    auto appEntity = dynamic_cast<AppFileEntity&&>(entity);

    std::unordered_map<HashType, std::string> hashes;
    for (auto& [hashType, hashValue] : appEntity.hashes)
    {
        hashes[HashTypeFromString(hashType, handler)] = std::move(hashValue);
    }

    std::vector<Architecture> architectures;
    for (auto& arch : appEntity.applicabilityDetails.architectures)
    {
        architectures.push_back(ArchitectureFromString(arch, handler));
    }

    std::unique_ptr<AppFile> tmp;
    THROW_IF_FAILED_LOG(AppFile::Make(std::move(appEntity.fileId),
                                      std::move(appEntity.url),
                                      appEntity.sizeInBytes,
                                      std::move(hashes),
                                      std::move(architectures),
                                      std::move(appEntity.applicabilityDetails.platformApplicabilityForPackage),
                                      std::move(appEntity.fileMoniker),
                                      tmp),
                        handler);
    return tmp;
}

std::vector<AppFile> AppFileEntity::FileEntitiesToAppFileVector(std::vector<std::unique_ptr<FileEntity>>&& entities,
                                                                const ReportingHandler& handler)
{
    std::vector<AppFile> tmp;
    for (auto& entity : entities)
    {
        tmp.push_back(std::move(*AppFileEntity::ToAppFile(std::move(*entity), handler)));
    }

    return tmp;
}
