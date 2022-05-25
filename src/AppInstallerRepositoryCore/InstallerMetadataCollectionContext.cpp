// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/InstallerMetadataCollectionContext.h"

#include <AppInstallerDownloader.h>
#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerStrings.h>

#include <winget/ManifestJSONParser.h>

using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Metadata
{

    void ProductMetadata::Clear()
    {
        m_version = {};
        m_productVersionMin = {};
        m_productVersionMax = {};
        m_installerMetadata.clear();
        m_historicalMetadata.clear();
    }

    void ProductMetadata::FromJson(const web::json::value& json)
    {
        Clear();

        utility::string_t versionFieldName = L"version";

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, json.is_null());

        auto versionString = AppInstaller::JSON::GetRawStringValueFromJsonNode(json, versionFieldName);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !versionString);

        m_version = Version{ versionString.value() };
        AICLI_LOG(Core, Info, << "Parsing metadata JSON version " << m_version.ToString());

        if (m_version.PartAt(0).Integer == 1)
        {
            // We only have one version currently, so use that as long as the major version is 1
            FromJson_1_0(json);
        }
        else
        {
            AICLI_LOG(Core, Error, << "Don't know how to handle metadata version " << m_version.ToString());
            THROW_HR(HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE));
        }
    }

    void ProductMetadata::FromJson_1_0(const web::json::value& json)
    {
        AICLI_LOG(Core, Info, << "Parsing metadata JSON 1.0 fields");

        // Field names
        utility::string_t productVersionMinFieldName = L"productVersionMin";
        utility::string_t productVersionMaxFieldName = L"productVersionMax";
        utility::string_t metadataFieldName = L"metadata";
        utility::string_t installerHashFieldName = L"installerHash";
        utility::string_t submissionIdentifierFieldName = L"submissionIdentifier";
        utility::string_t versionFieldName = L"version";
        utility::string_t appsAndFeaturesFieldName = L"AppsAndFeaturesEntries";
        utility::string_t historicalFieldName = L"historical";

        auto productVersionMinString = AppInstaller::JSON::GetRawStringValueFromJsonNode(json, productVersionMinFieldName);
        if (productVersionMinString)
        {
            m_productVersionMin = Version{ productVersionMinString.value() };
        }

        auto productVersionMaxString = AppInstaller::JSON::GetRawStringValueFromJsonNode(json, productVersionMaxFieldName);
        if (productVersionMaxString)
        {
            m_productVersionMax = Version{ productVersionMaxString.value() };
        }

        // The 1.0 version of metadata uses the 1.1 version of REST
        JSON::ManifestJSONParser parser{ Version{ "1.1" } };

        auto metadataArray = AppInstaller::JSON::GetRawJsonArrayFromJsonNode(json, metadataFieldName);
        if (metadataArray)
        {
            for (const auto& item : metadataArray->get())
            {
                auto installerHashString = AppInstaller::JSON::GetRawStringValueFromJsonNode(item, installerHashFieldName);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !installerHashString);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, m_installerMetadata.find(installerHashString.value()) != m_installerMetadata.end());

                InstallerMetadata installerMetadata;

                auto submissionIdentifierString = AppInstaller::JSON::GetRawStringValueFromJsonNode(item, submissionIdentifierFieldName);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !submissionIdentifierString);
                installerMetadata.SubmissionIdentifier = submissionIdentifierString.value();

                auto versionString = AppInstaller::JSON::GetRawStringValueFromJsonNode(item, versionFieldName);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !versionString);
                installerMetadata.ProductVersion = Version{ versionString.value() };

                auto appsAndFeatures = AppInstaller::JSON::GetRawJsonArrayFromJsonNode(item, appsAndFeaturesFieldName);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !appsAndFeatures);
                installerMetadata.AppsAndFeaturesEntries = parser.DeserializeAppsAndFeaturesEntries(appsAndFeatures.value());

                m_installerMetadata[installerHashString.value()] = std::move(installerMetadata);
            }
        }

        auto historicalArray = AppInstaller::JSON::GetRawJsonArrayFromJsonNode(json, historicalFieldName);
        if (historicalArray)
        {
            for (const auto& item : historicalArray->get())
            {
                HistoricalMetadata historicalMetadata;

                auto versionString = AppInstaller::JSON::GetRawStringValueFromJsonNode(item, versionFieldName);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !versionString);
                historicalMetadata.ProductVersion = Version{ versionString.value() };

                auto appsAndFeatures = AppInstaller::JSON::GetRawJsonArrayFromJsonNode(item, appsAndFeaturesFieldName);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !appsAndFeatures);
                historicalMetadata.AppsAndFeaturesEntries = parser.DeserializeAppsAndFeaturesEntries(appsAndFeatures.value());

                m_historicalMetadata.emplace_back(std::move(historicalMetadata));
            }
        }
    }

    std::unique_ptr<InstallerMetadataCollectionContext> InstallerMetadataCollectionContext::FromFile(const std::filesystem::path& file, const std::filesystem::path& logFile)
    {
        THROW_HR_IF(E_INVALIDARG, file.empty());
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !std::filesystem::exists(file));

        std::unique_ptr<InstallerMetadataCollectionContext> result = std::make_unique<InstallerMetadataCollectionContext>();
        auto threadGlobalsLifetime = result->InitializeLogging(logFile);

        AICLI_LOG(Core, Info, << "Opening InstallerMetadataCollectionContext input file: " << file);
        std::ifstream fileStream{ file };

        result->InitializePreinstallState(ConvertToUTF16(ReadEntireStream(fileStream)));

        return result;
    }

    std::unique_ptr<InstallerMetadataCollectionContext> InstallerMetadataCollectionContext::FromURI(std::wstring_view uri, const std::filesystem::path& logFile)
    {
        THROW_HR_IF(E_INVALIDARG, uri.empty());

        std::unique_ptr<InstallerMetadataCollectionContext> result = std::make_unique<InstallerMetadataCollectionContext>();
        auto threadGlobalsLifetime = result->InitializeLogging(logFile);

        std::string utf8Uri = ConvertToUTF8(uri);
        THROW_HR_IF(E_INVALIDARG, !IsUrlRemote(utf8Uri));

        AICLI_LOG(Core, Info, << "Downloading InstallerMetadataCollectionContext input file: " << utf8Uri);

        std::ostringstream jsonStream;
        ProgressCallback emptyCallback;

        const int MaxRetryCount = 2;
        for (int retryCount = 0; retryCount < MaxRetryCount; ++retryCount)
        {
            bool success = false;
            try
            {
                auto downloadHash = DownloadToStream(utf8Uri, jsonStream, DownloadType::InstallerMetadataCollectionInput, emptyCallback);

                success = true;
            }
            catch (...)
            {
                if (retryCount < MaxRetryCount - 1)
                {
                    AICLI_LOG(Core, Info, << "  Downloading InstallerMetadataCollectionContext input failed, waiting a bit and retrying...");
                    Sleep(500);
                }
                else
                {
                    throw;
                }
            }

            if (success)
            {
                break;
            }
        }

        result->InitializePreinstallState(ConvertToUTF16(jsonStream.str()));

        return result;
    }

    std::unique_ptr<InstallerMetadataCollectionContext> InstallerMetadataCollectionContext::FromJSON(std::wstring_view json, const std::filesystem::path& logFile)
    {
        THROW_HR_IF(E_INVALIDARG, json.empty());

        std::unique_ptr<InstallerMetadataCollectionContext> result = std::make_unique<InstallerMetadataCollectionContext>();
        auto threadGlobalsLifetime = result->InitializeLogging(logFile);
        result->InitializePreinstallState(std::wstring{ json });

        return result;
    }

    void InstallerMetadataCollectionContext::Complete(const std::filesystem::path& output)
    {
        auto threadGlobalsLifetime = m_threadGlobals.SetForCurrentThread();

        THROW_HR_IF(E_INVALIDARG, output.empty());

        // Collect post-install system state

        // Compute metadata match scores

        // Write output

        // Write diagnostics
    }

    std::unique_ptr<ThreadLocalStorage::PreviousThreadGlobals> InstallerMetadataCollectionContext::InitializeLogging(const std::filesystem::path& logFile)
    {
        auto threadGlobalsLifetime = m_threadGlobals.SetForCurrentThread();

        Logging::Log().SetLevel(Logging::Level::Info);
        Logging::Log().EnableChannel(Logging::Channel::All);
        Logging::EnableWilFailureTelemetry();
        Logging::AddTraceLogger();

        if (!logFile.empty())
        {
            Logging::AddFileLogger(logFile);
        }

        Logging::Telemetry().SetCaller("installer-metadata-collection");
        Logging::Telemetry().LogStartup();

        return threadGlobalsLifetime;
    }

    void InstallerMetadataCollectionContext::InitializePreinstallState(const std::wstring& json)
    {
        AICLI_LOG(Core, Verbose, << "Parsing input JSON:\n" << ConvertToUTF8(json));

        // Parse and validate JSON
        try
        {
            utility::string_t versionFieldName = L"version";

            web::json::value inputValue = web::json::value::parse(json);

            THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, inputValue.is_null());

            auto versionString = AppInstaller::JSON::GetRawStringValueFromJsonNode(inputValue, versionFieldName);
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !versionString);

            Version version{ versionString.value() };
            AICLI_LOG(Core, Info, << "Parsing input JSON version " << version.ToString());

            if (version.PartAt(0).Integer == 1)
            {
                // We only have one version currently, so use that as long as the major version is 1
                ParseInputJson_1_0(inputValue);
            }
            else
            {
                AICLI_LOG(Core, Error, << "Don't know how to handle version " << version.ToString());
                THROW_HR(HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE));
            }
        }
        catch (const web::json::json_exception& exc)
        {
            AICLI_LOG(Core, Error, << "Exception parsing input JSON: " << exc.what());
            throw;
        }

        // Collect pre-install system state
        m_correlationData.CapturePreInstallSnapshot();
    }

    void InstallerMetadataCollectionContext::ParseInputJson_1_0(web::json::value& input)
    {
        AICLI_LOG(Core, Info, << "Parsing input JSON 1.0 fields");

        // Field names
        utility::string_t metadataVersionFieldName = L"supportedMetadataVersion";
        utility::string_t metadataSizeFieldName = L"maximumMetadataSize";
        utility::string_t metadataFieldName = L"currentMetadata";
        utility::string_t submissionIdentifierFieldName = L"submissionIdentifier";
        utility::string_t installerHashFieldName = L"installerHash";
        utility::string_t currentManifestFieldName = L"currentManifest";
        utility::string_t submissionDataFieldName = L"submissionData";
        utility::string_t defaultLocaleFieldName = L"DefaultLocale";
        utility::string_t localesFieldName = L"Locales";

        auto metadataVersionString = AppInstaller::JSON::GetRawStringValueFromJsonNode(input, metadataVersionFieldName);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !metadataVersionString);
        m_supportedMetadataVersion = Version{ metadataVersionString.value() };

        auto metadataSizeNumber = AppInstaller::JSON::GetRawIntValueFromJsonNode(input, metadataSizeFieldName);
        if (metadataSizeNumber && metadataSizeNumber.value() > 0)
        {
            m_maxMetadataSize = static_cast<size_t>(metadataSizeNumber.value());
        }
        else
        {
            m_maxMetadataSize = std::numeric_limits<size_t>::max();
        }

        auto currentMetadataValue = AppInstaller::JSON::GetJsonValueFromNode(input, metadataFieldName);
        if (currentMetadataValue)
        {
            m_currentMetadata.FromJson(currentMetadataValue.value());
        }

        auto submissionIdentifierString = AppInstaller::JSON::GetRawStringValueFromJsonNode(input, submissionIdentifierFieldName);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !submissionIdentifierString);
        m_submissionIdentifier = submissionIdentifierString.value();

        auto installerHashString = AppInstaller::JSON::GetRawStringValueFromJsonNode(input, installerHashFieldName);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !installerHashString);
        m_installerHash = SHA256::ConvertToBytes(installerHashString.value());

        // The 1.0 version of input uses the 1.1 version of REST
        JSON::ManifestJSONParser parser{ Version{ "1.1" }};

        auto currentManifestValue = AppInstaller::JSON::GetJsonValueFromNode(input, currentManifestFieldName);
        if (currentManifestValue)
        {
            std::vector<Manifest::Manifest> manifests = parser.DeserializeData(currentManifestValue.value());

            if (!manifests.empty())
            {
                std::sort(manifests.begin(), manifests.end(), [](const Manifest::Manifest& a, const Manifest::Manifest& b) { return a.Version < b.Version; });
                // Latest version will be sorted to last position by Version < predicate
                m_currentManifest = std::move(manifests.back());
            }
        }

        auto submissionDataValue = AppInstaller::JSON::GetJsonValueFromNode(input, submissionDataFieldName);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !submissionDataValue);
        {
            auto defaultLocaleValue = AppInstaller::JSON::GetJsonValueFromNode(submissionDataValue.value(), defaultLocaleFieldName);
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !defaultLocaleValue);

            auto defaultLocale = parser.DeserializeLocale(defaultLocaleValue.value());
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE,
                !defaultLocale ||
                !defaultLocale->Contains(Manifest::Localization::PackageName) ||
                !defaultLocale->Contains(Manifest::Localization::Publisher));

            m_incomingManifest.DefaultLocalization = std::move(defaultLocale).value();

            auto localesArray = AppInstaller::JSON::GetRawJsonArrayFromJsonNode(submissionDataValue.value(), localesFieldName);
            if (localesArray)
            {
                for (const auto& locale : localesArray->get())
                {
                    auto localization = parser.DeserializeLocale(locale);
                    if (localization)
                    {
                        m_incomingManifest.Localizations.emplace_back(std::move(localization).value());
                    }
                }
            }
        }
    }
}
