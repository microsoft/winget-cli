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
#include <winget/UserSettings.h>
#include <winget/Filesystem.h>

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
        void TestHook_SetPathOverride(PathName target, const PathDetails& details);
        void TestHook_ClearPathOverrides();
    }

    namespace Repository
    {
        void TestHook_SetSourceFactoryOverride(const std::string& type, std::function<std::unique_ptr<ISourceFactory>()>&& factory);
        void TestHook_ClearSourceFactoryOverrides();
    }

    namespace Repository::Microsoft
    {
        void TestHook_SetPinningIndex_Override(std::optional<std::filesystem::path>&& indexPath);
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

    namespace WindowsFeature
    {
        void TestHook_SetEnableWindowsFeatureResult_Override(HRESULT* result);
        void TestHook_SetIsWindowsFeatureEnabledResult_Override(bool* status);
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

    struct SetEnableWindowsFeatureResult_Override
    {
        SetEnableWindowsFeatureResult_Override(HRESULT result) : m_result(result)
        {
            AppInstaller::WindowsFeature::TestHook_SetEnableWindowsFeatureResult_Override(&m_result);
        }

        ~SetEnableWindowsFeatureResult_Override()
        {
            AppInstaller::WindowsFeature::TestHook_SetEnableWindowsFeatureResult_Override(nullptr);
        }

    private:
        HRESULT m_result;
    };

    struct SetIsWindowsFeatureEnabledResult_Override
    {
        SetIsWindowsFeatureEnabledResult_Override(bool status) : m_status(status)
        {
            AppInstaller::WindowsFeature::TestHook_SetIsWindowsFeatureEnabledResult_Override(&m_status);
        }

        ~SetIsWindowsFeatureEnabledResult_Override()
        {
            AppInstaller::WindowsFeature::TestHook_SetIsWindowsFeatureEnabledResult_Override(nullptr);
        }

    private:
        bool m_status;
    };
}