// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/InstallerMetadataCollectionContext.h"

#include "Public/AppInstallerDownloader.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"

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
        // Parse and validate JSON
        web::json::value inputValue = web::json::value::parse(json);

        //if (inputValue.has_field)

        // Collect pre-install system state
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
}
