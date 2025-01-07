// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <SourceFactory.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

#include <AppInstallerTelemetry.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerDownloader.h>
#include <winget/UserSettings.h>
#include <winget/Filesystem.h>
#include <winget/IconExtraction.h>
#include <winget/Authentication.h>
#include <winget/HttpClientHelper.h>
#include <sfsclient/SFSClient.h>

#ifdef AICLI_DISABLE_TEST_HOOKS
static_assert(false, "Test hooks have been disabled");
#endif

namespace AppInstaller
{
    // Don't forget to clear the overrides after use!
    // A good way is to create a helper struct that cleans when destroyed

    namespace Runtime
    {
        void TestHook_SetPathOverride(PathName target, const std::filesystem::path& path);
        void TestHook_SetPathOverride(PathName target, const Filesystem::PathDetails& details);
        void TestHook_ClearPathOverrides();
    }

    namespace Repository
    {
        void TestHook_SetSourceFactoryOverride(const std::string& type, std::function<std::unique_ptr<ISourceFactory>()>&& factory);
        void TestHook_ClearSourceFactoryOverrides();
        void TestHook_SetExtractIconFromArpEntryResult_Override(std::vector<AppInstaller::Repository::ExtractedIconInfo>* result);
    }

    namespace Repository::Microsoft
    {
        void TestHook_SetPinningIndex_Override(std::optional<std::filesystem::path>&& indexPath);

        using GetARPKeyFunc = std::function<Registry::Key(Manifest::ScopeEnum, Utility::Architecture)>;
        void SetGetARPKeyOverride(GetARPKeyFunc value);
    }

    namespace Logging
    {
        void TestHook_SetTelemetryOverride(std::shared_ptr<TelemetryTraceLogger> ttl);
    }

    namespace Settings
    {
        void SetUserSettingsOverride(UserSettings* value);
    }

    namespace Filesystem
    {
        void TestHook_SetCreateSymlinkResult_Override(bool* status);
    }

    namespace Archive
    {
        void TestHook_SetScanArchiveResult_Override(bool* status);
    }

    namespace CLI::Workflow
    {
        void TestHook_SetEnableWindowsFeatureResult_Override(std::optional<DWORD>&& result);
        void TestHook_SetDoesWindowsFeatureExistResult_Override(std::optional<DWORD>&& result);
        void TestHook_SetExtractArchiveWithTarResult_Override(std::optional<DWORD>&& result);
    }

    namespace Reboot
    {
        void TestHook_SetInitiateRebootResult_Override(bool* status);
        void TestHook_SetRegisterForRestartResult_Override(bool* status);
    }

    namespace Authentication
    {
        void TestHook_SetAuthenticationResult_Override(Authentication::AuthenticationResult* authResult);
    }

    namespace MSStore::TestHooks
    {
        void SetDisplayCatalogHttpPipelineStage_Override(std::shared_ptr<web::http::http_pipeline_stage> value);

        void SetSfsClientAppContents_Override(std::function<std::vector<SFS::AppContent>(std::string_view)>* value);

        void SetLicensingHttpPipelineStage_Override(std::shared_ptr<web::http::http_pipeline_stage> value);
    }

    namespace Utility::TestHooks
    {
        void SetDownloadResult_Function_Override(std::function<DownloadResult(
            const std::string& url,
            const std::filesystem::path& dest,
            DownloadType type,
            IProgressCallback& progress,
            std::optional<DownloadInfo> info)>* value);
    }
}

namespace TestHook
{
    struct SetCreateSymlinkResult_Override
    {
        SetCreateSymlinkResult_Override(bool status) : m_status(status)
        {
            AppInstaller::Filesystem::TestHook_SetCreateSymlinkResult_Override(&m_status);
        }

        ~SetCreateSymlinkResult_Override()
        {
            AppInstaller::Filesystem::TestHook_SetCreateSymlinkResult_Override(nullptr);
        }

    private:
        bool m_status;
    };

    struct SetScanArchiveResult_Override
    {
        SetScanArchiveResult_Override(bool status) : m_status(status)
        {
            AppInstaller::Archive::TestHook_SetScanArchiveResult_Override(&m_status);
        }

        ~SetScanArchiveResult_Override()
        {
            AppInstaller::Archive::TestHook_SetScanArchiveResult_Override(nullptr);
        }

    private:
        bool m_status;
    };

    struct SetPinningIndex_Override
    {
        SetPinningIndex_Override(const std::filesystem::path& indexPath)
        {
            AppInstaller::Repository::Microsoft::TestHook_SetPinningIndex_Override(indexPath);
        }

        ~SetPinningIndex_Override()
        {
            AppInstaller::Repository::Microsoft::TestHook_SetPinningIndex_Override({});
        }
    };

    struct SetExtractIconFromArpEntryResult_Override
    {
        SetExtractIconFromArpEntryResult_Override(std::vector<AppInstaller::Repository::ExtractedIconInfo> extractedIcons) : m_extractedIcons(std::move(extractedIcons))
        {
            AppInstaller::Repository::TestHook_SetExtractIconFromArpEntryResult_Override(&m_extractedIcons);
        }

        ~SetExtractIconFromArpEntryResult_Override()
        {
            AppInstaller::Repository::TestHook_SetExtractIconFromArpEntryResult_Override(nullptr);
        }

    private:
        std::vector<AppInstaller::Repository::ExtractedIconInfo> m_extractedIcons;
    };

    struct SetEnableWindowsFeatureResult_Override
    {
        SetEnableWindowsFeatureResult_Override(DWORD result)
        {
            AppInstaller::CLI::Workflow::TestHook_SetEnableWindowsFeatureResult_Override(result);
        }

        ~SetEnableWindowsFeatureResult_Override()
        {
            AppInstaller::CLI::Workflow::TestHook_SetEnableWindowsFeatureResult_Override({});
        }
    };

    struct SetDoesWindowsFeatureExistResult_Override
    {
        SetDoesWindowsFeatureExistResult_Override(DWORD result)
        {
            AppInstaller::CLI::Workflow::TestHook_SetDoesWindowsFeatureExistResult_Override(result);
        }

        ~SetDoesWindowsFeatureExistResult_Override()
        {
            AppInstaller::CLI::Workflow::TestHook_SetDoesWindowsFeatureExistResult_Override({});
        }
    };

    struct SetExtractArchiveWithTarResult_Override
    {
        SetExtractArchiveWithTarResult_Override(DWORD result)
        {
            AppInstaller::CLI::Workflow::TestHook_SetExtractArchiveWithTarResult_Override(result);
        }

        ~SetExtractArchiveWithTarResult_Override()
        {
            AppInstaller::CLI::Workflow::TestHook_SetExtractArchiveWithTarResult_Override({});
        }
    };

    struct SetInitiateRebootResult_Override
    {
        SetInitiateRebootResult_Override(bool status) : m_status(status)
        {
            AppInstaller::Reboot::TestHook_SetInitiateRebootResult_Override(&m_status);
        }

        ~SetInitiateRebootResult_Override()
        {
            AppInstaller::Reboot::TestHook_SetInitiateRebootResult_Override(nullptr);
        }

    private:
        bool m_status;
    };

    struct SetGetARPKey_Override
    {
        SetGetARPKey_Override(std::function<AppInstaller::Registry::Key(AppInstaller::Manifest::ScopeEnum, AppInstaller::Utility::Architecture)> function)
        {
            AppInstaller::Repository::Microsoft::SetGetARPKeyOverride(function);
        }

        ~SetGetARPKey_Override()
        {
            AppInstaller::Repository::Microsoft::SetGetARPKeyOverride({});
        }

    private:
    };

    struct SetRegisterForRestartResult_Override
    {
        SetRegisterForRestartResult_Override(bool status) : m_status(status)
        {
            AppInstaller::Reboot::TestHook_SetRegisterForRestartResult_Override(&m_status);
        }

        ~SetRegisterForRestartResult_Override()
        {
            AppInstaller::Reboot::TestHook_SetRegisterForRestartResult_Override(nullptr);
        }

    private:
        bool m_status;
    };

    struct SetAuthenticationResult_Override
    {
        SetAuthenticationResult_Override(AppInstaller::Authentication::AuthenticationResult authResult) : m_authResult(authResult)
        {
            AppInstaller::Authentication::TestHook_SetAuthenticationResult_Override(&m_authResult);
        }

        ~SetAuthenticationResult_Override()
        {
            AppInstaller::Authentication::TestHook_SetAuthenticationResult_Override(nullptr);
        }

    private:
        AppInstaller::Authentication::AuthenticationResult m_authResult;
    };

    struct SetDisplayCatalogHttpPipelineStage_Override
    {
        SetDisplayCatalogHttpPipelineStage_Override(std::shared_ptr<web::http::http_pipeline_stage> value)
        {
            AppInstaller::MSStore::TestHooks::SetDisplayCatalogHttpPipelineStage_Override(value);
        }

        ~SetDisplayCatalogHttpPipelineStage_Override()
        {
            AppInstaller::MSStore::TestHooks::SetDisplayCatalogHttpPipelineStage_Override(nullptr);
        }
    };

    struct SetSfsClientAppContents_Override
    {
        SetSfsClientAppContents_Override(std::function<std::vector<SFS::AppContent>(std::string_view)> value) : m_appContentsFunction(std::move(value))
        {
            AppInstaller::MSStore::TestHooks::SetSfsClientAppContents_Override(&m_appContentsFunction);
        }

        ~SetSfsClientAppContents_Override()
        {
            AppInstaller::MSStore::TestHooks::SetSfsClientAppContents_Override(nullptr);
        }

    private:
        std::function<std::vector<SFS::AppContent>(std::string_view)> m_appContentsFunction;
    };

    struct SetLicensingHttpPipelineStage_Override
    {
        SetLicensingHttpPipelineStage_Override(std::shared_ptr<web::http::http_pipeline_stage> value)
        {
            AppInstaller::MSStore::TestHooks::SetLicensingHttpPipelineStage_Override(value);
        }

        ~SetLicensingHttpPipelineStage_Override()
        {
            AppInstaller::MSStore::TestHooks::SetLicensingHttpPipelineStage_Override(nullptr);
        }
    };

    struct SetDownloadResult_Function_Override
    {
        SetDownloadResult_Function_Override(std::function<AppInstaller::Utility::DownloadResult(
            const std::string& url,
            const std::filesystem::path& dest,
            AppInstaller::Utility::DownloadType type,
            AppInstaller::IProgressCallback& progress,
            std::optional<AppInstaller::Utility::DownloadInfo> info)> value) : m_downloadFunction(std::move(value))
        {
            AppInstaller::Utility::TestHooks::SetDownloadResult_Function_Override(&m_downloadFunction);
        }

        ~SetDownloadResult_Function_Override()
        {
            AppInstaller::Utility::TestHooks::SetDownloadResult_Function_Override(nullptr);
        }

    private:
        std::function<AppInstaller::Utility::DownloadResult(
            const std::string& url,
            const std::filesystem::path& dest,
            AppInstaller::Utility::DownloadType type,
            AppInstaller::IProgressCallback& progress,
            std::optional<AppInstaller::Utility::DownloadInfo> info)> m_downloadFunction;
    };
}
