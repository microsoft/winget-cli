// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <AppInstallerVersions.h>
#include <winget/Manifest.h>
#include <winget/JsonUtil.h>
#include <winget/ThreadGlobals.h>
#include <winget/ARPCorrelation.h>
#include <winget/InstalledFilesCorrelation.h>
#include <winget/IconExtraction.h>

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
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

        // Create a JSON value for the metadata using the given schema version.
        web::json::value ToJson(const Utility::Version& schemaVersion, size_t maximumSizeInBytes);

        // Copies the metadata from the source. If the given submission identifier does not match
        // the source, it's data is moved to historical.
        void CopyFrom(const ProductMetadata& source, std::string_view submissionIdentifier);

        // The installer specific metadata that we collect.
        struct InstallerMetadata
        {
            friend ProductMetadata;

            // 1.0
            std::string SubmissionIdentifier;
            std::vector<Manifest::AppsAndFeaturesEntry> AppsAndFeaturesEntries;

            // 1.1
            // If Scope value is empty, the value is not set before. If the value is Unknown, a conflicting value is encountered.
            std::string Scope;

            // 1.2
            // If std::nullopt, the value is not set before. If the value is empty(i.e. !HasData()), a conflicting value is encountered.
            std::optional<Manifest::InstallationMetadataInfo> InstalledFiles;
            // If std::nullopt, the value is not set before. If the vector is empty, conflicting values are encountered.
            std::optional<std::vector<Correlation::InstalledStartupLinkFile>> StartupLinkFiles;
            // Extracted icons
            std::vector<ExtractedIconInfo> Icons;
        };

        // Metadata from previous product revisions.
        struct HistoricalMetadata
        {
            // 1.0
            Utility::Version ProductVersionMin;
            Utility::Version ProductVersionMax;
            std::set<std::string> Names;
            std::set<std::string> Publishers;
            std::set<std::string> ProductCodes;
            std::set<std::string> UpgradeCodes;
        };

        // 1.0
        Utility::Version SchemaVersion;
        Utility::Version ProductVersionMin;
        Utility::Version ProductVersionMax;
        // Map from installer hash to metadata
        std::map<std::string, InstallerMetadata> InstallerMetadataMap;
        std::vector<HistoricalMetadata> HistoricalMetadataList;

    private:
        void FromJson_1_N(const web::json::value& json);
        web::json::value ToJson_1_N();

        // Removes the historical data with the oldest version.
        // Returns true if something was removed; false it not.
        bool DropOldestHistoricalData();
    };

    // Contains the functions and data used for collecting metadata from installers.
    struct InstallerMetadataCollectionContext
    {
        InstallerMetadataCollectionContext();
        InstallerMetadataCollectionContext(
            std::unique_ptr<Correlation::ARPCorrelationData> correlationData,
            std::unique_ptr<Correlation::InstalledFilesCorrelation> installedFilesCorrelation,
            const std::wstring& json);

        InstallerMetadataCollectionContext(const InstallerMetadataCollectionContext&) = delete;
        InstallerMetadataCollectionContext& operator=(const InstallerMetadataCollectionContext&) = delete;

        InstallerMetadataCollectionContext(InstallerMetadataCollectionContext&&) = default;
        InstallerMetadataCollectionContext& operator=(InstallerMetadataCollectionContext&&) = default;

        // Create from various forms of JSON input to prevent type collisions on constructor.
        static std::unique_ptr<InstallerMetadataCollectionContext> FromFile(const std::filesystem::path& file, const std::filesystem::path& logFile);
        static std::unique_ptr<InstallerMetadataCollectionContext> FromURI(std::wstring_view uri, const std::filesystem::path& logFile);
        static std::unique_ptr<InstallerMetadataCollectionContext> FromJSON(const std::wstring& json, const std::filesystem::path& logFile);

        // Completes the collection, writing to the given location.
        void Complete(const std::filesystem::path& output);

        // Completes the collection, writing to the given location.
        void Complete(std::ostream& output);

        static std::wstring Merge(const std::wstring& json, size_t maximumSizeInBytes, const std::filesystem::path& logFile);

    private:
        // Initializes the context runtime, including the log file if provided.
        static std::unique_ptr<ThreadLocalStorage::PreviousThreadGlobals> InitializeLogging(ThreadLocalStorage::WingetThreadGlobals& threadGlobals, const std::filesystem::path& logFile);
        std::unique_ptr<ThreadLocalStorage::PreviousThreadGlobals> InitializeLogging(const std::filesystem::path& logFile);

        // Sets the collection context input and the preinstall state.
        void InitializePreinstallState(const std::wstring& json);

        // Creates the output ProductMetadata and diagnostics objects for output
        void ComputeOutputData();

        // Callers should set the thread globals before calling this.
        void CompleteWithThreadGlobalsSet(std::ostream& output);

        // Parse version 1.0 of input JSON
        void ParseInputJson_1_0(web::json::value& input);

        // Create version 1.0 of output JSON
        web::json::value CreateOutputJson_1_0();

        // Determines whether an error has occurred in the context.
        bool ContainsError() const;

        // Collects information from the exception for error reporting.
        void CollectErrorDataFromException(std::exception_ptr exception);

        // Create version 1.0 of error JSON
        web::json::value CreateErrorJson_1_0();

        // Merge using merge input version 1.0
        static web::json::value Merge_1_0(web::json::value& input, size_t maximumSizeInBytes);

        ThreadLocalStorage::WingetThreadGlobals m_threadGlobals;

        // Parsed input
        Utility::Version m_inputVersion;
        Utility::Version m_supportedMetadataVersion;
        ProductMetadata m_currentMetadata;
        web::json::value m_submissionData;
        std::string m_submissionIdentifier;
        std::string m_installerHash;
        Manifest::Manifest m_incomingManifest;

        std::unique_ptr<Correlation::ARPCorrelationData> m_correlationData;
        std::unique_ptr<Correlation::InstalledFilesCorrelation> m_installedFilesCorrelation;

        // Output data
        enum class OutputStatus
        {
            // Version 1.0 status values
            Unknown,
            Success,
            Error,
            LowConfidence,
        };

        // Convert status to a JSON string value
        static utility::string_t ToString(OutputStatus status);

        OutputStatus m_outputStatus = OutputStatus::Unknown;
        ProductMetadata m_outputMetadata;
        web::json::value m_outputDiagnostics;

        // Error data storage
        HRESULT m_errorHR = S_OK;
        std::string m_errorText;
    };
}
