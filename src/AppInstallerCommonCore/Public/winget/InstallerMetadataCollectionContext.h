// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <Public/AppInstallerVersions.h>
#include <Public/AppInstallerSHA256.h>
#include <winget/Manifest.h>
#include <winget/JsonUtil.h>
#include <winget/ThreadGlobals.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace AppInstaller::Utility
{
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
        void Complete(const std::filesystem::path& output, const std::filesystem::path& diagnostics);

    private:
        // Initializes the context runtime, including the log file if provided.
        std::unique_ptr<ThreadLocalStorage::PreviousThreadGlobals> InitializeLogging(const std::filesystem::path& logFile);

        // Sets the collection context input and the preinstall state.
        void InitializePreinstallState(const std::wstring& json);

        // Parse version 1.0 of input JSON
        void ParseInputJson_1_0(web::json::value& input);

        ThreadLocalStorage::ThreadGlobals m_threadGlobals;

        // Parsed input
        Version m_supportedBlobVersion;
        size_t m_maxBlobSize = 0;
        web::json::value m_currentBlob; // TODO: Parse blob as well
        int m_productRevision = 0;
        SHA256::HashBuffer m_installerHash;
        Manifest::Manifest m_currentManifest;
        Manifest::Manifest m_incomingManifest;
    };
}
