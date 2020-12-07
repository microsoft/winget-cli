// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallFlow.h"
#include "UninstallFlow.h"
#include "Resources.h"
#include "ShellExecuteInstallerHandler.h"
#include "MSStoreInstallerHandler.h"
#include "WorkflowBase.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace winrt::Windows::ApplicationModel::Store::Preview::InstallControl;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::Management::Deployment;
    using namespace AppInstaller::Utility;
    using namespace AppInstaller::Manifest;
    using namespace AppInstaller::Repository;

    void EnsureMinOSVersion(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();

        if (!manifest.MinOSVersion.empty() &&
            !Runtime::IsCurrentOSVersionGreaterThanOrEqual(Version(manifest.MinOSVersion)))
        {
            context.Reporter.Error() << Resource::String::InstallationRequiresHigherWindows << ' ' << manifest.MinOSVersion << std::endl;
            AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION));
        }
    }

    void EnsureApplicableInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>();

        if (!installer.has_value())
        {
            context.Reporter.Error() << Resource::String::NoApplicableInstallers << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);
        }
    }

    void ShowInstallationDisclaimer(Execution::Context& context)
    {
        auto installerType = context.Get<Execution::Data::Installer>().value().InstallerType;

        if (installerType == ManifestInstaller::InstallerTypeEnum::MSStore)
        {
            context.Reporter.Info() << Resource::String::InstallationDisclaimerMSStore << std::endl;
        }
        else
        {
            context.Reporter.Info() <<
                Resource::String::InstallationDisclaimer1 << std::endl <<
                Resource::String::InstallationDisclaimer2 << std::endl;
        }
    }

    void DownloadInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>().value();

        switch (installer.InstallerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Inno:
        case ManifestInstaller::InstallerTypeEnum::Msi:
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
        case ManifestInstaller::InstallerTypeEnum::Wix:
            context << DownloadInstallerFile << VerifyInstallerHash << UpdateInstallerFileMotwIfApplicable;
            break;
        case ManifestInstaller::InstallerTypeEnum::Msix:
            if (installer.SignatureSha256.empty())
            {
                context << DownloadInstallerFile << VerifyInstallerHash << UpdateInstallerFileMotwIfApplicable;
            }
            else
            {
                // Signature hash provided. No download needed. Just verify signature hash.
                context << GetMsixSignatureHash << VerifyInstallerHash << UpdateInstallerFileMotwIfApplicable;
            }
            break;
        case ManifestInstaller::InstallerTypeEnum::MSStore:
            // Nothing to do here
            break;
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void DownloadInstallerFile(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        const auto& installer = context.Get<Execution::Data::Installer>().value();

        std::filesystem::path tempInstallerPath = Runtime::GetPathTo(Runtime::PathName::Temp);
        tempInstallerPath /= Utility::ConvertToUTF16(manifest.Id + '.' + manifest.Version);

        AICLI_LOG(CLI, Info, << "Generated temp download path: " << tempInstallerPath);

        context.Reporter.Info() << "Downloading " << Execution::UrlEmphasis << installer.Url << std::endl;

        std::optional<std::vector<BYTE>> hash;

        const int MaxRetryCount = 2;
        for (int retryCount = 0; retryCount < MaxRetryCount; ++retryCount)
        {
            bool success = false;
            try
            {
                hash = context.Reporter.ExecuteWithProgress(std::bind(Utility::Download,
                    installer.Url,
                    tempInstallerPath,
                    std::placeholders::_1,
                    true));

                success = true;
            }
            catch (...)
            {
                if (retryCount < MaxRetryCount - 1)
                {
                    AICLI_LOG(CLI, Info, << "Failed to download, waiting a bit and retry. Url: " << installer.Url);
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

        if (!hash)
        {
            context.Reporter.Info() << "Package download canceled." << std::endl;
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }

        context.Add<Execution::Data::HashPair>(std::make_pair(installer.Sha256, hash.value()));
        context.Add<Execution::Data::InstallerPath>(std::move(tempInstallerPath));
    }

    void GetMsixSignatureHash(Execution::Context& context)
    {
        // We use this when the server won't support streaming install to swap to download.
        bool downloadInstead = false;

        try
        {
            const auto& installer = context.Get<Execution::Data::Installer>().value();

            Msix::MsixInfo msixInfo(installer.Url);
            auto signature = msixInfo.GetSignature();

            auto signatureHash = SHA256::ComputeHash(signature.data(), static_cast<uint32_t>(signature.size()));

            context.Add<Execution::Data::HashPair>(std::make_pair(installer.SignatureSha256, signatureHash));
        }
        catch (const winrt::hresult_error& e)
        {
            if (e.code() == HRESULT_FROM_WIN32(ERROR_NO_RANGES_PROCESSED) ||
                HRESULT_FACILITY(e.code()) == FACILITY_HTTP)
            {
                // Failed to get signature hash through HttpStream, use download
                downloadInstead = true;
            }
            else
            {
                throw;
            }
        }

        if (downloadInstead)
        {
            context << DownloadInstallerFile;
        }
    }

    void VerifyInstallerHash(Execution::Context& context)
    {
        const auto& hashPair = context.Get<Execution::Data::HashPair>();

        if (!std::equal(
            hashPair.first.begin(),
            hashPair.first.end(),
            hashPair.second.begin()))
        {
            bool overrideHashMismatch = context.Args.Contains(Execution::Args::Type::Force);

            const auto& manifest = context.Get<Execution::Data::Manifest>();
            Logging::Telemetry().LogInstallerHashMismatch(manifest.Id, manifest.Version, manifest.Channel, hashPair.first, hashPair.second, overrideHashMismatch);

            // If running as admin, do not allow the user to override the hash failure.
            if (Runtime::IsRunningAsAdmin())
            {
                context.Reporter.Error() << Resource::String::InstallerHashMismatchAdminBlock << std::endl;
            }
            else if (overrideHashMismatch)
            {
                context.Reporter.Warn() << Resource::String::InstallerHashMismatchOverridden << std::endl;
                return;
            }
            else
            {
                context.Reporter.Error() << Resource::String::InstallerHashMismatchOverrideRequired << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALLER_HASH_MISMATCH);
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Installer hash verified");
            context.Reporter.Info() << Resource::String::InstallerHashVerified << std::endl;

            context.SetFlags(Execution::ContextFlag::InstallerHashMatched);

            if (context.Contains(Execution::Data::PackageVersion) &&
                context.Get<Execution::Data::PackageVersion>()->GetSource() != nullptr &&
                SourceTrustLevel::Trusted == context.Get<Execution::Data::PackageVersion>()->GetSource()->GetDetails().TrustLevel)
            {
                context.SetFlags(Execution::ContextFlag::InstallerTrusted);
            }
        }
    }

    void UpdateInstallerFileMotwIfApplicable(Execution::Context& context)
    {
        if (context.Contains(Execution::Data::InstallerPath))
        {
            if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerTrusted))
            {
                Utility::ApplyMotwIfApplicable(context.Get<Execution::Data::InstallerPath>(), URLZONE_TRUSTED);
            }
            else if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerHashMatched))
            {
                const auto& installer = context.Get<Execution::Data::Installer>();
                HRESULT hr = Utility::ApplyMotwUsingIAttachmentExecuteIfApplicable(context.Get<Execution::Data::InstallerPath>(), installer.value().Url);

                // Not using SUCCEEDED(hr) to check since there are cases file is missing after a successful scan
                if (hr != S_OK)
                {
                    switch (hr)
                    {
                    case INET_E_SECURITY_PROBLEM:
                        context.Reporter.Error() << Resource::String::InstallerBlockedByPolicy << std::endl;
                        break;
                    case E_FAIL:
                        context.Reporter.Error() << Resource::String::InstallerFailedVirusScan << std::endl;
                        break;
                    default:
                        context.Reporter.Error() << Resource::String::InstallerFailedSecurityCheck << std::endl;
                    }

                    AICLI_LOG(Fail, Error, << "Installer failed security check. Url: " << installer.value().Url << " Result: " << WINGET_OSTREAM_FORMAT_HRESULT(hr));
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALLER_SECURITY_CHECK_FAILED);
                }
            }
        }
    }

    void ExecuteInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>().value();

        bool isUpdate = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate);

        switch (installer.InstallerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Inno:
        case ManifestInstaller::InstallerTypeEnum::Msi:
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
        case ManifestInstaller::InstallerTypeEnum::Wix:
            if (isUpdate && installer.UpdateBehavior == ManifestInstaller::UpdateBehaviorEnum::UninstallPrevious)
            {
                context <<
                    GetUninstallInfo <<
                    ExecuteUninstaller;
                context.ClearFlags(Execution::ContextFlag::InstallerExecutionUseUpdate);
            }
            context << ShellExecuteInstall;
            break;
        case ManifestInstaller::InstallerTypeEnum::Msix:
            context << MsixInstall;
            break;
        case ManifestInstaller::InstallerTypeEnum::MSStore:
            context <<
                EnsureFeatureEnabled(Settings::ExperimentalFeature::Feature::ExperimentalMSStore) <<
                EnsureStorePolicySatisfied <<
                (isUpdate ? MSStoreUpdate : MSStoreInstall);
            break;
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void ShellExecuteInstall(Execution::Context& context)
    {
        context <<
            GetInstallerArgs <<
            RenameDownloadedInstaller <<
            ShellExecuteInstallImpl;
    }

    void MsixInstall(Execution::Context& context)
    {
        Uri uri = nullptr;
        if (context.Contains(Execution::Data::InstallerPath))
        {
            uri = Uri(context.Get<Execution::Data::InstallerPath>().c_str());
        }
        else
        {
            uri = Uri(Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->Url));
        }

        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        try
        {
            DeploymentOptions deploymentOptions =
                DeploymentOptions::ForceApplicationShutdown |
                DeploymentOptions::ForceTargetApplicationShutdown;

            context.Reporter.ExecuteWithProgress(std::bind(Deployment::AddPackage, uri, deploymentOptions,
                WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerTrusted), std::placeholders::_1));
        }
        catch (const wil::ResultException& re)
        {
            const auto& manifest = context.Get<Execution::Data::Manifest>();
            Logging::Telemetry().LogInstallerFailure(manifest.Id, manifest.Version, manifest.Channel, "MSIX", re.GetErrorCode());

            context.Reporter.Error() << GetUserPresentableMessage(re) << std::endl;
            AICLI_TERMINATE_CONTEXT(re.GetErrorCode());
        }

        context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
    }

    void RemoveInstaller(Execution::Context& context)
    {
        // Path may not be present if installed from a URL for MSIX
        if (context.Contains(Execution::Data::InstallerPath))
        {
            const auto& path = context.Get<Execution::Data::InstallerPath>();
            AICLI_LOG(CLI, Info, << "Removing installer: " << path);

            try
            {
                // best effort
                std::filesystem::remove(path);
            }
            catch (const std::exception& e)
            {
                AICLI_LOG(CLI, Warning, << "Failed to remove installer file after execution. Reason: " << e.what());
            }
            catch (...)
            {
                AICLI_LOG(CLI, Warning, << "Failed to remove installer file after execution. Reason unknown.");
            }
        }
    }
}
