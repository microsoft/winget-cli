// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include <winget/ThreadGlobals.h>

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

        ThreadLocalStorage::ThreadGlobals m_threadGlobals;
    };
}
