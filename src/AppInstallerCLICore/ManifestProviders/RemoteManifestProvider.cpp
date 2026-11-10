// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RemoteManifestProvider.h"
#include "ManifestArchive.h"
#include "ExecutionContext.h"
#include <AppInstallerDownloader.h>
#include <winget/ManifestYamlParser.h>
#include <AppInstallerRuntime.h>

namespace AppInstaller::CLI::Workflow
{
    RemoteManifestProvider::RemoteManifestProvider(std::string_view argument) : m_argument(argument) {}

    Settings::BoolAdminSetting RemoteManifestProvider::RequiredAdminSetting() const
    {
        return Settings::BoolAdminSetting::RemoteManifestFiles;
    }

    std::optional<Manifest::Manifest> RemoteManifestProvider::Load(Execution::Context& context) const
    {
        Logging::Telemetry().LogIsManifestLocal(false);
        if (!Settings::IsAdminSettingEnabled(RequiredAdminSetting()))
        {
            context.Reporter.Error() << Resource::String::FeatureDisabledByAdminSettingMessage(
                Settings::AdminSettingToString(RequiredAdminSetting())) << std::endl;
            AICLI_TERMINATE_CONTEXT_RETURN(E_ACCESSDENIED, std::nullopt);
        }

        std::filesystem::path temporaryDirectory;
        auto cleanup = wil::scope_exit([&]
        {
            if (!temporaryDirectory.empty())
            {
                std::error_code error;
                std::filesystem::remove_all(temporaryDirectory, error);
            }
        });
        auto manifestPath = Download(context, temporaryDirectory);
        if (manifestPath.extension() == ".zip")
        {
            return LoadManifestArchive(manifestPath);
        }
        return Manifest::YamlParser::CreateFromPath(manifestPath);
    }

    std::filesystem::path RemoteManifestProvider::Download(
        Execution::Context& context, std::filesystem::path& temporaryDirectory) const
    {
        THROW_HR_IF(E_INVALIDARG, !web::uri::validate(Utility::ConvertToUTF16(m_argument)));
        web::uri uri{ Utility::ConvertToUTF16(m_argument) };
        const auto extension = Utility::ToLower(std::filesystem::path{ uri.path() }.extension().u8string());
        THROW_HR_IF(E_INVALIDARG,
            !Utility::CaseInsensitiveEquals(Utility::ConvertToUTF8(uri.scheme()), "https") ||
            uri.host().empty() || !uri.user_info().empty() || !uri.query().empty() || !uri.fragment().empty() ||
            (uri.port() != 0 && uri.port() != 443) ||
            (extension != ".zip" && extension != ".yaml" && extension != ".yml"));

        temporaryDirectory = Runtime::GetNewTempFilePath();
        THROW_HR_IF(E_UNEXPECTED, !std::filesystem::create_directory(temporaryDirectory));
        auto manifestPath = temporaryDirectory / ("manifest" + extension);
        context.Reporter.ExecuteWithProgress([&](IProgressCallback& progress)
        {
            THROW_HR_IF(E_ABORT, progress.IsCancelledBy(CancelReason::Any));
            Utility::Download(m_argument, manifestPath, Utility::DownloadType::Manifest, progress);
            THROW_HR_IF(E_ABORT, progress.IsCancelledBy(CancelReason::Any));
        });
        return manifestPath;
    }

}
