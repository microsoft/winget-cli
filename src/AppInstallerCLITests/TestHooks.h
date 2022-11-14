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
}
