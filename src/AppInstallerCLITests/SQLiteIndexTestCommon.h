// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Microsoft/SQLiteIndex.h>
#include <winget/Manifest.h>
#include <winget/SQLiteWrapper.h>


// Fixture helpers shared by the index test files.
namespace TestCommon
{
    using SQLiteVersion = AppInstaller::SQLite::Version;

    // Creates an index to test against.
    // When no version is given, the test is generated once for each of the last few schema
    // versions, so callers that care about a specific version must name it.
    AppInstaller::Repository::Microsoft::SQLiteIndex CreateTestIndex(const std::string& filePath, std::optional<SQLiteVersion> version = {});

    // Gets the relative path that the given manifest would be stored at.
    std::string GetPathFromManifest(AppInstaller::Manifest::Manifest& manifest);

    // Fills in a manifest with values derived from the given publisher.
    void CreateFakeManifest(
        AppInstaller::Manifest::Manifest& manifest,
        AppInstaller::Manifest::string_t publisher,
        AppInstaller::Manifest::string_t version = "1.0.0");

    // A manifest along with the path that it is stored at.
    struct ManifestAndPath
    {
        AppInstaller::Manifest::Manifest Manifest;
        std::string Path;
    };

    // Fills in a manifest and gives it a unique path, along with the hash of that path.
    void CreateFakeManifestAndPath(
        ManifestAndPath& manifestAndPath,
        const AppInstaller::Manifest::string_t& publisher,
        std::string_view version = "1.0.0",
        std::optional<std::string_view> arpMinVersion = {},
        std::optional<std::string_view> arpMaxVersion = {});

    // The values that a test places into an index for a single manifest.
    struct IndexFields
    {
        IndexFields(
            std::string id,
            std::string name,
            std::string moniker,
            std::string version,
            std::string channel,
            std::vector<AppInstaller::Utility::NormalizedString> tags,
            std::vector<AppInstaller::Utility::NormalizedString> commands,
            std::string path
        ) :
            Id(std::move(id)),
            Name(std::move(name)),
            Moniker(std::move(moniker)),
            Version(std::move(version)),
            Channel(std::move(channel)),
            Tags(std::move(tags)),
            Commands(std::move(commands)),
            Path(std::move(path))
        {}

        IndexFields(
            std::string id,
            std::string name,
            std::string moniker,
            std::string version,
            std::string channel,
            std::vector<AppInstaller::Utility::NormalizedString> tags,
            std::vector<AppInstaller::Utility::NormalizedString> commands,
            std::string path,
            std::vector<AppInstaller::Utility::NormalizedString> packageFamilyNames,
            std::vector<AppInstaller::Utility::NormalizedString> productCodes
        ) :
            Id(std::move(id)),
            Name(std::move(name)),
            Moniker(std::move(moniker)),
            Version(std::move(version)),
            Channel(std::move(channel)),
            Tags(std::move(tags)),
            Commands(std::move(commands)),
            Path(std::move(path)),
            PackageFamilyNames(std::move(packageFamilyNames)),
            ProductCodes(std::move(productCodes))
        {}

        IndexFields(
            std::string id,
            std::string name,
            std::string publisher,
            std::string moniker,
            std::string version,
            std::string channel,
            std::vector<AppInstaller::Utility::NormalizedString> tags,
            std::vector<AppInstaller::Utility::NormalizedString> commands,
            std::string path,
            std::vector<AppInstaller::Utility::NormalizedString> packageFamilyNames,
            std::vector<AppInstaller::Utility::NormalizedString> productCodes
        ) :
            Id(std::move(id)),
            Name(std::move(name)),
            Publisher(std::move(publisher)),
            Moniker(std::move(moniker)),
            Version(std::move(version)),
            Channel(std::move(channel)),
            Tags(std::move(tags)),
            Commands(std::move(commands)),
            Path(std::move(path)),
            PackageFamilyNames(std::move(packageFamilyNames)),
            ProductCodes(std::move(productCodes))
        {}

        IndexFields(
            std::string id,
            std::string name,
            std::string publisher,
            std::string moniker,
            std::string version,
            std::string channel,
            std::vector<AppInstaller::Utility::NormalizedString> tags,
            std::vector<AppInstaller::Utility::NormalizedString> commands,
            std::string path,
            std::vector<AppInstaller::Utility::NormalizedString> packageFamilyNames,
            std::vector<AppInstaller::Utility::NormalizedString> productCodes,
            std::string arpName,
            std::string arpPublisher
        ) :
            Id(std::move(id)),
            Name(std::move(name)),
            Publisher(std::move(publisher)),
            Moniker(std::move(moniker)),
            Version(std::move(version)),
            Channel(std::move(channel)),
            Tags(std::move(tags)),
            Commands(std::move(commands)),
            Path(std::move(path)),
            PackageFamilyNames(std::move(packageFamilyNames)),
            ProductCodes(std::move(productCodes)),
            ArpName(std::move(arpName)),
            ArpPublisher(std::move(arpPublisher))
        {}

        std::string Id;
        std::string Name;
        std::string Publisher;
        std::string Moniker;
        std::string Version;
        std::string Channel;
        std::vector<AppInstaller::Utility::NormalizedString> Tags;
        std::vector<AppInstaller::Utility::NormalizedString> Commands;
        std::string Path;
        std::vector<AppInstaller::Utility::NormalizedString> PackageFamilyNames;
        std::vector<AppInstaller::Utility::NormalizedString> ProductCodes;
        std::string ArpName;
        std::string ArpPublisher;
    };

    // Applies the given fields to a manifest, overwriting what it names and leaving the rest.
    void ApplyIndexFields(AppInstaller::Manifest::Manifest& manifest, const IndexFields& fields);

    // Produces the manifest described by the given fields.
    AppInstaller::Manifest::Manifest CreateManifest(const IndexFields& fields);

    // Creates an index containing the given data.
    AppInstaller::Repository::Microsoft::SQLiteIndex SearchTestSetup(
        const std::string& filePath,
        std::initializer_list<IndexFields> data = {},
        std::optional<SQLiteVersion> version = {});
}
