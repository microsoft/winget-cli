// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/InstallerMetadataCollectionContext.h"

#include "Public/AppInstallerDownloader.h"
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"

// TODO: Move to self to repository because we can't have a layering inversion
#include <winget/ManifestJSONParser.h>

namespace AppInstaller::Utility
{
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

    void InstallerMetadataCollectionContext::Complete(const std::filesystem::path& output, const std::filesystem::path& diagnostics)
    {
        auto threadGlobalsLifetime = m_threadGlobals.SetForCurrentThread();

        THROW_HR_IF(E_INVALIDARG, output.empty());

        UNREFERENCED_PARAMETER(diagnostics);

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

            auto versionString = JSON::GetRawStringValueFromJsonNode(inputValue, versionFieldName);
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !versionString);

            Version version{ versionString.value() };
            AICLI_LOG(Core, Info, << "Parsing input JSON version " << version.ToString());

            if (version.GetParts()[0].Integer == 1)
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
    }

    void InstallerMetadataCollectionContext::ParseInputJson_1_0(web::json::value& input)
    {
        AICLI_LOG(Core, Info, << "Parsing input JSON 1.0 fields");

        // Field names
        utility::string_t blobVersionFieldName = L"supportedBlobVersion";
        utility::string_t blobSizeFieldName = L"maximumBlobSize";
        utility::string_t blobFieldName = L"currentBlob";
        utility::string_t productRevisionFieldName = L"productRevision";
        utility::string_t installerHashFieldName = L"installerHash";
        utility::string_t currentManifestFieldName = L"currentManifest";
        utility::string_t incomingManifestFieldName = L"incomingManifest";

        auto blobVersionString = JSON::GetRawStringValueFromJsonNode(input, blobVersionFieldName);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !blobVersionString);
        m_supportedBlobVersion = Version{ blobVersionString.value() };

        auto blobSizeNumber = JSON::GetRawIntValueFromJsonNode(input, blobSizeFieldName);
        if (blobSizeNumber && blobSizeNumber.value() > 0)
        {
            m_maxBlobSize = static_cast<size_t>(blobSizeNumber.value());
        }
        else
        {
            m_maxBlobSize = std::numeric_limits<size_t>::max();
        }

        auto currentBlobValue = JSON::GetJsonValueFromNode(input, blobFieldName);
        if (currentBlobValue)
        {
            // TODO: Create and parse blob here
            m_currentBlob = currentBlobValue.value();
        }

        auto productRevisionNumber = JSON::GetRawIntValueFromJsonNode(input, productRevisionFieldName);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !productRevisionNumber);
        m_productRevision = productRevisionNumber.value();

        auto installerHashString = JSON::GetRawStringValueFromJsonNode(input, installerHashFieldName);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !installerHashString);
        m_installerHash = SHA256::ConvertToBytes(installerHashString.value());

        auto currentManifestValue = JSON::GetJsonValueFromNode(input, currentManifestFieldName);
        if (currentManifestValue)
        {
            //m_currentManifest = 
        }

        auto incomingManifestValue = JSON::GetJsonValueFromNode(input, incomingManifestFieldName);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, !incomingManifestValue);
        // m_incomingManifest = 
    }
}
