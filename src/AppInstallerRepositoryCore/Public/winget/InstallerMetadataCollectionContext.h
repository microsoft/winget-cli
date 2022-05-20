// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <AppInstallerVersions.h>
#include <AppInstallerSHA256.h>
#include <winget/Manifest.h>
#include <winget/JsonUtil.h>
#include <winget/ThreadGlobals.h>

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace AppInstaller::Repository::Metadata
{
    // The overall metadata that we collect.
    struct ProductMetadata
    {
        ProductMetadata() = default;

        // Removes all stored data.
        void Clear();

        // Load the metadata from an existing JSON blob.
        void FromJson(const web::json::value& json);

        // The installer specific metadata that we collect.
        struct InstallerMetadata
        {
            int ProductRevision;
            Utility::Version ProductVersion;
            std::vector<Manifest::AppsAndFeaturesEntry> AppsAndFeaturesEntries;
        };

        // Metadata from previous product revisions.
        struct HistoricalMetadata
        {
            Utility::Version ProductVersion;
            std::vector<Manifest::AppsAndFeaturesEntry> AppsAndFeaturesEntries;
        };

    private:
        void FromJson_1_0(const web::json::value& json);

        Utility::Version m_version;
        Utility::Version m_productVersionMin;
        Utility::Version m_productVersionMax;
        // Map from installer hash to metadata
        std::map<std::string, InstallerMetadata> m_installerMetadata;
        std::vector<HistoricalMetadata> m_historicalMetadata;
    };

    // Contains the functions and data used for collecting metadata from installers.
    struct InstallerMetadataCollectionContext
    {
        InstallerMetadataCollectionContext() = default;

        InstallerMetadataCollectionContext(const InstallerMetadataCollectionContext&) = delete;
        InstallerMetadataCollectionContext& operator=(const InstallerMetadataCollectionContext&) = delete;

        InstallerMetadataCollectionContext(InstallerMetadataCollectionContext&&) = default;
        InstallerMetadataCollectionContext& operator=(InstallerMetadataCollectionContext&&) = default;

        // Create from various forms of JSON input to prevent type collisions on constructor.
        static std::unique_ptr<InstallerMetadataCollectionContext> FromFile(const std::filesystem::path& file, const std::filesystem::path& logFile);
        static std::unique_ptr<InstallerMetadataCollectionContext> FromURI(std::wstring_view uri, const std::filesystem::path& logFile);
        static std::unique_ptr<InstallerMetadataCollectionContext> FromJSON(std::wstring_view json, const std::filesystem::path& logFile);

        // Completes the collection, writing to the given locations.
        void Complete(const std::filesystem::path& output);

    private:
        // Initializes the context runtime, including the log file if provided.
        std::unique_ptr<ThreadLocalStorage::PreviousThreadGlobals> InitializeLogging(const std::filesystem::path& logFile);

        // Sets the collection context input and the preinstall state.
        void InitializePreinstallState(const std::wstring& json);

        // Parse version 1.0 of input JSON
        void ParseInputJson_1_0(web::json::value& input);

        ThreadLocalStorage::ThreadGlobals m_threadGlobals;

        // Parsed input
        Utility::Version m_supportedMetadataVersion;
        size_t m_maxMetadataSize = 0;
        ProductMetadata m_currentMetadata;
        int m_productRevision = 0;
        Utility::SHA256::HashBuffer m_installerHash;
        Manifest::Manifest m_currentManifest;
        Manifest::Manifest m_incomingManifest;
    };
}
