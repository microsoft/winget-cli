// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "ContentType.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json_fwd.hpp>

namespace SFS
{
class File;
class AppFile;

namespace details
{
class ReportingHandler;

struct FileEntity;

using FileEntities = std::vector<std::unique_ptr<FileEntity>>;

struct FileEntity
{
    virtual ~FileEntity()
    {
    }

    virtual ContentType GetContentType() const = 0;

    std::string fileId;
    std::string url;
    uint64_t sizeInBytes;
    std::unordered_map<std::string, std::string> hashes;

    static std::unique_ptr<FileEntity> FromJson(const nlohmann::json& file, const ReportingHandler& handler);
    static FileEntities DownloadInfoResponseToFileEntities(const nlohmann::json& data, const ReportingHandler& handler);
};

struct GenericFileEntity : public FileEntity
{
    ContentType GetContentType() const override;

    static std::unique_ptr<File> ToFile(FileEntity&& entity, const ReportingHandler& handler);
    static std::vector<File> FileEntitiesToFileVector(FileEntities&& entities, const ReportingHandler& handler);
};

struct ApplicabilityDetailsEntity
{
    std::vector<std::string> architectures;
    std::vector<std::string> platformApplicabilityForPackage;
};

struct AppFileEntity : public FileEntity
{
    ContentType GetContentType() const override;

    std::string fileMoniker;
    ApplicabilityDetailsEntity applicabilityDetails;

    static std::unique_ptr<AppFile> ToAppFile(FileEntity&& entity, const ReportingHandler& handler);
    static std::vector<AppFile> FileEntitiesToAppFileVector(FileEntities&& entities, const ReportingHandler& handler);
};

} // namespace details
} // namespace SFS
