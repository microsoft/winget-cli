// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "../../../util/SFSExceptionMatcher.h"
#include "../../../util/TestHelper.h"
#include "ReportingHandler.h"
#include "entity/FileEntity.h"
#include "sfsclient/AppFile.h"
#include "sfsclient/File.h"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#define TEST(...) TEST_CASE("[FileEntityTests] " __VA_ARGS__)

using namespace SFS;
using namespace SFS::details;
using namespace SFS::test;
using json = nlohmann::json;

// GenericFileEntity constants
const std::string c_fileId = "fileId";
const std::string c_url = "url";
const uint64_t c_size = 123;
const std::string c_sha1 = "sha1";
const std::string c_sha256 = "sha256";

// AppFileEntity constants
const std::string c_fileMoniker = "fileMoniker";
const std::string c_arch = "amd64";
const std::string c_applicability = "app";
ApplicabilityDetailsEntity c_appDetailsEntity{{c_arch}, {c_applicability}};

TEST("Testing FileEntity::FromJson()")
{
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);

    SECTION("Generic File Entity")
    {
        std::unique_ptr<FileEntity> entity;
        SECTION("Correct")
        {
            const json fileEntity = {{"FileId", c_fileId},
                                     {"Url", c_url},
                                     {"SizeInBytes", c_size},
                                     {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}}};

            REQUIRE_NOTHROW(entity = FileEntity::FromJson(fileEntity, handler));
            REQUIRE(entity != nullptr);
            REQUIRE(entity->GetContentType() == ContentType::Generic);
            REQUIRE(entity->fileId == c_fileId);
            REQUIRE(entity->url == c_url);
            REQUIRE(entity->sizeInBytes == c_size);
            REQUIRE(entity->hashes.size() == 2);
            REQUIRE(entity->hashes.at("Sha1") == c_sha1);
            REQUIRE(entity->hashes.at("Sha256") == c_sha256);
        }

        SECTION("Missing fields")
        {
            SECTION("Missing FileId")
            {
                const json fileEntity = {{"Url", c_url},
                                         {"SizeInBytes", c_size},
                                         {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}}};
                REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing File.FileId in response");
            }

            SECTION("Missing Url")
            {
                const json fileEntity = {{"FileId", c_fileId},
                                         {"SizeInBytes", c_size},
                                         {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}}};
                REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing File.Url in response");
            }

            SECTION("Missing SizeInBytes")
            {
                const json fileEntity = {{"FileId", c_fileId},
                                         {"Url", c_url},
                                         {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}}};
                REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing File.SizeInBytes in response");
            }

            SECTION("Missing Hashes")
            {
                const json fileEntity = {{"FileId", c_fileId}, {"Url", c_url}, {"SizeInBytes", c_size}};
                REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing File.Hashes in response");
            }
        }

        SECTION("Wrong types")
        {
            SECTION("FileId")
            {
                const json fileEntity = {{"FileId", 1},
                                         {"Url", c_url},
                                         {"SizeInBytes", c_size},
                                         {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}}};
                REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                        ServiceInvalidResponse,
                                        "File.FileId is not a string");
            }

            SECTION("Url")
            {
                const json fileEntity = {{"FileId", c_fileId},
                                         {"Url", 1},
                                         {"SizeInBytes", c_size},
                                         {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}}};
                REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                        ServiceInvalidResponse,
                                        "File.Url is not a string");
            }

            SECTION("SizeInBytes")
            {
                const json fileEntity = {{"FileId", c_fileId},
                                         {"Url", c_url},
                                         {"SizeInBytes", "size"},
                                         {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}}};
                REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                        ServiceInvalidResponse,
                                        "File.SizeInBytes is not an unsigned number");
            }

            SECTION("Hashes not an object")
            {
                const json fileEntity = {{"FileId", c_fileId}, {"Url", c_url}, {"SizeInBytes", c_size}, {"Hashes", 1}};
                REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                        ServiceInvalidResponse,
                                        "File.Hashes is not an object");
            }

            SECTION("Hashes.Sha1")
            {
                const json fileEntity = {{"FileId", c_fileId},
                                         {"Url", c_url},
                                         {"SizeInBytes", c_size},
                                         {"Hashes", {{"Sha1", 1}}}};
                REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                        ServiceInvalidResponse,
                                        "File.Hashes object value is not a string");
            }
        }
    }

    SECTION("App File Entity")
    {
        std::unique_ptr<FileEntity> entity;
        const json applicabilityDetails = {{"Architectures", json::array({c_arch})},
                                           {"PlatformApplicabilityForPackage", json::array({c_applicability})}};

        SECTION("Correct")
        {
            const json fileEntity = {{"FileId", c_fileId},
                                     {"Url", c_url},
                                     {"SizeInBytes", c_size},
                                     {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}},
                                     {"FileMoniker", c_fileMoniker},
                                     {"ApplicabilityDetails", applicabilityDetails}};

            REQUIRE_NOTHROW(entity = FileEntity::FromJson(fileEntity, handler));
            REQUIRE(entity != nullptr);
            REQUIRE(entity->GetContentType() == ContentType::App);
            REQUIRE(entity->fileId == c_fileId);
            REQUIRE(entity->url == c_url);
            REQUIRE(entity->sizeInBytes == c_size);
            REQUIRE(entity->hashes.size() == 2);
            REQUIRE(entity->hashes.at("Sha1") == c_sha1);
            REQUIRE(entity->hashes.at("Sha256") == c_sha256);

            AppFileEntity* appEntity = dynamic_cast<AppFileEntity*>(entity.get());
            REQUIRE(appEntity->fileMoniker == c_fileMoniker);
            REQUIRE(appEntity->applicabilityDetails.architectures.size() == 1);
            REQUIRE(appEntity->applicabilityDetails.architectures[0] == c_arch);
            REQUIRE(appEntity->applicabilityDetails.platformApplicabilityForPackage.size() == 1);
            REQUIRE(appEntity->applicabilityDetails.platformApplicabilityForPackage[0] == c_applicability);
        }

        SECTION("Missing fields")
        {
            SECTION("Missing ApplicabilityDetails")
            {
                const json fileEntity = {{"FileId", c_fileId},
                                         {"Url", c_url},
                                         {"SizeInBytes", c_size},
                                         {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}},
                                         {"FileMoniker", c_fileMoniker}};
                REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing File.ApplicabilityDetails in response");
            }

            SECTION("Missing ApplicabilityDetails.Architectures")
            {
                const json wrongApplicabilityDetails = {
                    {"PlatformApplicabilityForPackage", json::array({c_applicability})}};
                const json fileEntity = {{"FileId", c_fileId},
                                         {"Url", c_url},
                                         {"SizeInBytes", c_size},
                                         {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}},
                                         {"FileMoniker", c_fileMoniker},
                                         {"ApplicabilityDetails", wrongApplicabilityDetails}};
                REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                        ServiceInvalidResponse,
                                        "Missing File.ApplicabilityDetails.Architectures in response");
            }

            SECTION("Missing ApplicabilityDetails.PlatformApplicabilityForPackage")
            {
                const json wrongApplicabilityDetails = {{"Architectures", json::array({c_arch})}};
                const json fileEntity = {{"FileId", c_fileId},
                                         {"Url", c_url},
                                         {"SizeInBytes", c_size},
                                         {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}},
                                         {"FileMoniker", c_fileMoniker},
                                         {"ApplicabilityDetails", wrongApplicabilityDetails}};
                REQUIRE_THROWS_CODE_MSG(
                    FileEntity::FromJson(fileEntity, handler),
                    ServiceInvalidResponse,
                    "Missing File.ApplicabilityDetails.PlatformApplicabilityForPackage in response");
            }
        }

        SECTION("Wrong types")
        {
            SECTION("FileMoniker")
            {
                const json fileEntity = {{"FileId", c_fileId},
                                         {"Url", c_url},
                                         {"SizeInBytes", c_size},
                                         {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}},
                                         {"FileMoniker", 1},
                                         {"ApplicabilityDetails", applicabilityDetails}};
                REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                        ServiceInvalidResponse,
                                        "File.FileMoniker is not a string");
            }

            SECTION("ApplicabilityDetails")
            {
                SECTION("Not an object")
                {
                    const json fileEntity = {{"FileId", c_fileId},
                                             {"Url", c_url},
                                             {"SizeInBytes", c_size},
                                             {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}},
                                             {"FileMoniker", c_fileMoniker},
                                             {"ApplicabilityDetails", c_fileId}};
                    REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                            ServiceInvalidResponse,
                                            "File.ApplicabilityDetails is not an object");
                }

                SECTION("Architectures is not an array")
                {
                    const json wrongApplicabilityDetails = {
                        {"Architectures", c_fileId},
                        {"PlatformApplicabilityForPackage", json::array({c_applicability})}};
                    const json fileEntity = {{"FileId", c_fileId},
                                             {"Url", c_url},
                                             {"SizeInBytes", c_size},
                                             {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}},
                                             {"FileMoniker", c_fileMoniker},
                                             {"ApplicabilityDetails", wrongApplicabilityDetails}};
                    REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                            ServiceInvalidResponse,
                                            "File.ApplicabilityDetails.Architectures is not an array");
                }

                SECTION("Architectures array value is not a string")
                {
                    const json wrongApplicabilityDetails = {
                        {"Architectures", json::array({1})},
                        {"PlatformApplicabilityForPackage", json::array({c_applicability})}};
                    const json fileEntity = {{"FileId", c_fileId},
                                             {"Url", c_url},
                                             {"SizeInBytes", c_size},
                                             {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}},
                                             {"FileMoniker", c_fileMoniker},
                                             {"ApplicabilityDetails", wrongApplicabilityDetails}};
                    REQUIRE_THROWS_CODE_MSG(FileEntity::FromJson(fileEntity, handler),
                                            ServiceInvalidResponse,
                                            "File.ApplicabilityDetails.Architectures array value is not a string");
                }

                SECTION("PlatformApplicabilityForPackage is not an array")
                {
                    const json wrongApplicabilityDetails = {{"Architectures", json::array({c_arch})},
                                                            {"PlatformApplicabilityForPackage", c_fileId}};
                    const json fileEntity = {{"FileId", c_fileId},
                                             {"Url", c_url},
                                             {"SizeInBytes", c_size},
                                             {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}},
                                             {"FileMoniker", c_fileMoniker},
                                             {"ApplicabilityDetails", wrongApplicabilityDetails}};
                    REQUIRE_THROWS_CODE_MSG(
                        FileEntity::FromJson(fileEntity, handler),
                        ServiceInvalidResponse,
                        "File.ApplicabilityDetails.PlatformApplicabilityForPackage is not an array");
                }

                SECTION("PlatformApplicabilityForPackage array value is not a string")
                {
                    const json wrongApplicabilityDetails = {{"Architectures", json::array({c_arch})},
                                                            {"PlatformApplicabilityForPackage", json::array({1})}};
                    const json fileEntity = {{"FileId", c_fileId},
                                             {"Url", c_url},
                                             {"SizeInBytes", c_size},
                                             {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}},
                                             {"FileMoniker", c_fileMoniker},
                                             {"ApplicabilityDetails", wrongApplicabilityDetails}};
                    REQUIRE_THROWS_CODE_MSG(
                        FileEntity::FromJson(fileEntity, handler),
                        ServiceInvalidResponse,
                        "File.ApplicabilityDetails.PlatformApplicabilityForPackage array value is not a string");
                }
            }
        }
    }
}

TEST("Testing GenericFileEntity conversions")
{
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);

    auto CheckFile = [&](const File& file) {
        REQUIRE(file.GetFileId() == c_fileId);
        REQUIRE(file.GetUrl() == c_url);
        REQUIRE(file.GetSizeInBytes() == c_size);
        REQUIRE(file.GetHashes().size() == 2);
        REQUIRE(file.GetHashes().at(HashType::Sha1) == c_sha1);
        REQUIRE(file.GetHashes().at(HashType::Sha256) == c_sha256);
    };

    SECTION("GenericFileEntity::ToFile()")
    {
        SECTION("Success")
        {
            std::unique_ptr<FileEntity> entity = std::make_unique<GenericFileEntity>();
            entity->fileId = c_fileId;
            entity->url = c_url;
            entity->sizeInBytes = c_size;
            entity->hashes = {{"Sha1", c_sha1}, {"Sha256", c_sha256}};

            auto file = GenericFileEntity::ToFile(std::move(*entity), handler);
            CheckFile(*file);
        }

        SECTION("Wrong type")
        {
            std::unique_ptr<FileEntity> wrongEntity = std::make_unique<AppFileEntity>();
            wrongEntity->fileId = c_fileId;
            wrongEntity->url = c_url;
            wrongEntity->sizeInBytes = c_size;
            wrongEntity->hashes = {{"Sha1", c_sha1}, {"Sha256", c_sha256}};

            REQUIRE_THROWS_CODE_MSG(
                GenericFileEntity::ToFile(std::move(*wrongEntity), handler),
                ServiceUnexpectedContentType,
                R"(The service returned file "fileId" with content type [App] while the expected type was [Generic])");
        }
    }

    SECTION("GenericFileEntity::FileEntitiesToFileVector()")
    {
        SECTION("Success")
        {
            std::unique_ptr<FileEntity> entity = std::make_unique<GenericFileEntity>();
            entity->fileId = c_fileId;
            entity->url = c_url;
            entity->sizeInBytes = c_size;
            entity->hashes = {{"Sha1", c_sha1}, {"Sha256", c_sha256}};

            FileEntities entities;
            entities.push_back(std::move(entity));

            std::vector<File> files;
            SECTION("1 file")
            {
                REQUIRE_NOTHROW(files = GenericFileEntity::FileEntitiesToFileVector(std::move(entities), handler));
                REQUIRE(files.size() == 1);
                CheckFile(files[0]);
            }

            SECTION("2 files")
            {
                std::unique_ptr<FileEntity> entity2 = std::make_unique<GenericFileEntity>();
                entity2->fileId = c_fileId;
                entity2->url = c_url;
                entity2->sizeInBytes = c_size;
                entity2->hashes = {{"Sha1", c_sha1}, {"Sha256", c_sha256}};

                entities.push_back(std::move(entity2));
                REQUIRE_NOTHROW(files = GenericFileEntity::FileEntitiesToFileVector(std::move(entities), handler));
                REQUIRE(files.size() == 2);
                CheckFile(files[0]);
                CheckFile(files[1]);
            }
        }

        SECTION("Wrong type, fails")
        {
            std::unique_ptr<FileEntity> wrongEntity = std::make_unique<AppFileEntity>();
            wrongEntity->fileId = c_fileId;
            wrongEntity->url = c_url;
            wrongEntity->sizeInBytes = c_size;
            wrongEntity->hashes = {{"Sha1", c_sha1}, {"Sha256", c_sha256}};

            FileEntities wrongEntities;
            wrongEntities.push_back(std::move(wrongEntity));
            REQUIRE_THROWS_CODE_MSG(
                GenericFileEntity::FileEntitiesToFileVector(std::move(wrongEntities), handler),
                ServiceUnexpectedContentType,
                R"(The service returned file "fileId" with content type [App] while the expected type was [Generic])");
        }
    }
}

TEST("Testing AppFileEntity conversions")
{
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);

    auto CheckAppFile = [&](const AppFile& file) {
        REQUIRE(file.GetFileId() == c_fileId);
        REQUIRE(file.GetUrl() == c_url);
        REQUIRE(file.GetSizeInBytes() == c_size);
        REQUIRE(file.GetHashes().size() == 2);
        REQUIRE(file.GetHashes().at(HashType::Sha1) == c_sha1);
        REQUIRE(file.GetHashes().at(HashType::Sha256) == c_sha256);
        REQUIRE(file.GetFileMoniker() == c_fileMoniker);
        REQUIRE(file.GetApplicabilityDetails().GetArchitectures().size() == 1);
        REQUIRE(file.GetApplicabilityDetails().GetArchitectures()[0] == Architecture::Amd64);
        REQUIRE(file.GetApplicabilityDetails().GetPlatformApplicabilityForPackage().size() == 1);
        REQUIRE(file.GetApplicabilityDetails().GetPlatformApplicabilityForPackage()[0] == c_applicability);
    };

    SECTION("AppFileEntity::ToAppFile()")
    {
        SECTION("Success")
        {
            std::unique_ptr<FileEntity> entity = std::make_unique<AppFileEntity>();
            auto appEntity = dynamic_cast<AppFileEntity*>(entity.get());
            appEntity->fileId = c_fileId;
            appEntity->url = c_url;
            appEntity->sizeInBytes = c_size;
            appEntity->hashes = {{"Sha1", c_sha1}, {"Sha256", c_sha256}};
            appEntity->fileMoniker = c_fileMoniker;
            appEntity->applicabilityDetails = c_appDetailsEntity;

            auto file = AppFileEntity::ToAppFile(std::move(*entity), handler);
            CheckAppFile(*file);
        }

        SECTION("Wrong type")
        {
            std::unique_ptr<FileEntity> wrongEntity = std::make_unique<GenericFileEntity>();
            wrongEntity->fileId = c_fileId;
            wrongEntity->url = c_url;
            wrongEntity->sizeInBytes = c_size;
            wrongEntity->hashes = {{"Sha1", c_sha1}, {"Sha256", c_sha256}};

            REQUIRE_THROWS_CODE_MSG(
                AppFileEntity::ToAppFile(std::move(*wrongEntity), handler),
                ServiceUnexpectedContentType,
                R"(The service returned file "fileId" with content type [Generic] while the expected type was [App])");
        }
    }

    SECTION("Testing AppFileEntity::FileEntitiesToAppFileVector()")
    {
        SECTION("Success")
        {
            std::unique_ptr<FileEntity> entity = std::make_unique<AppFileEntity>();
            auto appEntity = dynamic_cast<AppFileEntity*>(entity.get());
            appEntity->fileId = c_fileId;
            appEntity->url = c_url;
            appEntity->sizeInBytes = c_size;
            appEntity->hashes = {{"Sha1", c_sha1}, {"Sha256", c_sha256}};
            appEntity->fileMoniker = c_fileMoniker;
            appEntity->applicabilityDetails = c_appDetailsEntity;

            FileEntities entities;
            entities.push_back(std::move(entity));

            std::vector<AppFile> files;
            SECTION("1 file")
            {
                REQUIRE_NOTHROW(files = AppFileEntity::FileEntitiesToAppFileVector(std::move(entities), handler));
                REQUIRE(files.size() == 1);
                CheckAppFile(files[0]);
            }

            SECTION("2 files")
            {
                std::unique_ptr<FileEntity> entity2 = std::make_unique<AppFileEntity>();
                auto appEntity2 = dynamic_cast<AppFileEntity*>(entity2.get());
                appEntity2->fileId = c_fileId;
                appEntity2->url = c_url;
                appEntity2->sizeInBytes = c_size;
                appEntity2->hashes = {{"Sha1", c_sha1}, {"Sha256", c_sha256}};
                appEntity2->fileMoniker = c_fileMoniker;
                appEntity2->applicabilityDetails = c_appDetailsEntity;

                entities.push_back(std::move(entity2));
                REQUIRE_NOTHROW(files = AppFileEntity::FileEntitiesToAppFileVector(std::move(entities), handler));
                REQUIRE(files.size() == 2);
                CheckAppFile(files[0]);
                CheckAppFile(files[1]);
            }
        }

        SECTION("Wrong type, fails")
        {
            std::unique_ptr<FileEntity> wrongEntity = std::make_unique<GenericFileEntity>();
            wrongEntity->fileId = c_fileId;
            wrongEntity->url = c_url;
            wrongEntity->sizeInBytes = c_size;
            wrongEntity->hashes = {{"Sha1", c_sha1}, {"Sha256", c_sha256}};

            FileEntities wrongEntities;
            wrongEntities.push_back(std::move(wrongEntity));
            REQUIRE_THROWS_CODE_MSG(
                AppFileEntity::FileEntitiesToAppFileVector(std::move(wrongEntities), handler),
                ServiceUnexpectedContentType,
                R"(The service returned file "fileId" with content type [Generic] while the expected type was [App])");
        }
    }
}

TEST("Testing FileEntity::DownloadInfoResponseToFileEntities()")
{
    ReportingHandler handler;
    handler.SetLoggingCallback(LogCallbackToTest);

    SECTION("GenericFileEntity")
    {
        auto CheckEntity = [&](const FileEntity& entity) {
            REQUIRE(entity.GetContentType() == ContentType::Generic);
            REQUIRE(entity.fileId == c_fileId);
            REQUIRE(entity.url == c_url);
            REQUIRE(entity.sizeInBytes == c_size);
            REQUIRE(entity.hashes.size() == 2);
            REQUIRE(entity.hashes.at("Sha1") == c_sha1);
            REQUIRE(entity.hashes.at("Sha256") == c_sha256);
        };

        SECTION("Correct")
        {
            const json fileJson = {{"FileId", c_fileId},
                                   {"Url", c_url},
                                   {"SizeInBytes", c_size},
                                   {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}}};

            SECTION("1 file")
            {
                const json downloadInfoResponse = json::array({fileJson});
                auto entities = FileEntity::DownloadInfoResponseToFileEntities(downloadInfoResponse, handler);
                REQUIRE(entities.size() == 1);
                REQUIRE(entities[0] != nullptr);
                CheckEntity(*entities[0]);
            }

            SECTION("2 files")
            {
                const json downloadInfoResponse = json::array({fileJson, fileJson});
                auto entities = FileEntity::DownloadInfoResponseToFileEntities(downloadInfoResponse, handler);
                REQUIRE(entities.size() == 2);
                REQUIRE(entities[0] != nullptr);
                CheckEntity(*entities[0]);
                REQUIRE(entities[1] != nullptr);
                CheckEntity(*entities[1]);
            }
        }
    }

    SECTION("AppFileEntity")
    {
        const json applicabilityDetails = {{"Architectures", json::array({c_arch})},
                                           {"PlatformApplicabilityForPackage", json::array({c_applicability})}};

        auto CheckEntity = [&](const AppFileEntity& entity) {
            REQUIRE(entity.GetContentType() == ContentType::App);
            REQUIRE(entity.fileId == c_fileId);
            REQUIRE(entity.url == c_url);
            REQUIRE(entity.sizeInBytes == c_size);
            REQUIRE(entity.hashes.size() == 2);
            REQUIRE(entity.hashes.at("Sha1") == c_sha1);
            REQUIRE(entity.hashes.at("Sha256") == c_sha256);
            REQUIRE(entity.fileMoniker == c_fileMoniker);
            REQUIRE(entity.applicabilityDetails.architectures.size() == 1);
            REQUIRE(entity.applicabilityDetails.architectures[0] == c_arch);
            REQUIRE(entity.applicabilityDetails.platformApplicabilityForPackage.size() == 1);
            REQUIRE(entity.applicabilityDetails.platformApplicabilityForPackage[0] == c_applicability);
        };

        SECTION("Correct")
        {
            const json fileJson = {{"FileId", c_fileId},
                                   {"Url", c_url},
                                   {"SizeInBytes", c_size},
                                   {"Hashes", {{"Sha1", c_sha1}, {"Sha256", c_sha256}}},
                                   {"FileMoniker", c_fileMoniker},
                                   {"ApplicabilityDetails", applicabilityDetails}};

            SECTION("1 file")
            {
                const json downloadInfoResponse = json::array({fileJson});
                auto entities = FileEntity::DownloadInfoResponseToFileEntities(downloadInfoResponse, handler);
                REQUIRE(entities.size() == 1);
                REQUIRE(entities[0] != nullptr);
                CheckEntity(dynamic_cast<AppFileEntity&>(*entities[0]));
            }

            SECTION("2 files")
            {
                const json downloadInfoResponse = json::array({fileJson, fileJson});
                auto entities = FileEntity::DownloadInfoResponseToFileEntities(downloadInfoResponse, handler);
                REQUIRE(entities.size() == 2);
                REQUIRE(entities[0] != nullptr);
                CheckEntity(dynamic_cast<AppFileEntity&>(*entities[0]));
                REQUIRE(entities[1] != nullptr);
                CheckEntity(dynamic_cast<AppFileEntity&>(*entities[1]));
            }
        }
    }
}
