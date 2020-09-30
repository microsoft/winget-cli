// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallFlow.h"
#include "Resources.h"
#include "ShellExecuteInstallerHandler.h"
#include "WorkflowBase.h"
#include<stdlib.h>
#include<stdio.h>
#include<PathCch.h>


#include <ShObjIdl_core.h>



//#include <windows.management.deployment.h>

namespace AppInstaller::CLI::Workflow
{
    using namespace winrt::Windows::ApplicationModel::Store::Preview::InstallControl;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::Management::Deployment;
    //using namespace winrt::Windows::Web;
    using namespace AppInstaller::Utility;
    using namespace AppInstaller::Manifest;

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
        case ManifestInstaller::InstallerTypeEnum::PWA:
            context << EnsureFeatureEnabled(Settings::ExperimentalFeature::Feature::ExperimentalPWA) << GeneratePwaMsix;
            break;
        case ManifestInstaller::InstallerTypeEnum::Exe:
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Inno:
        case ManifestInstaller::InstallerTypeEnum::Msi:
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
        case ManifestInstaller::InstallerTypeEnum::Wix:
            context << DownloadInstallerFile << VerifyInstallerHash;
            break;
        case ManifestInstaller::InstallerTypeEnum::Msix:
            if (installer.SignatureSha256.empty())
            {
                context << DownloadInstallerFile << VerifyInstallerHash;
            }
            else
            {
                // Signature hash provided. No download needed. Just verify signature hash.
                context << GetMsixSignatureHash << VerifyInstallerHash;
            }
            break;
        case ManifestInstaller::InstallerTypeEnum::MSStore:
            // Nothing to do here
            break;
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }


    void PWAInstall(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;
        const auto& path = context.Get<Execution::Data::InstallerPath>();
        Uri uri(path.c_str());
        PackageManager packageManager;
        AddPackageOptions options;
        options.AllowUnsigned(true);
        options.DeferRegistrationWhenPackagesAreInUse(true);
        IPackageManager9 packageManager9 = packageManager.as<IPackageManager9>();
        auto result = packageManager9.AddPackageByUriAsync(uri, options).get();
            
       if (!result.ErrorText().empty())
       {
           //Not sure how to report this error
           context.Reporter.Error() << to_string(result.ErrorText()) << std::endl;
           AICLI_TERMINATE_CONTEXT(result.ExtendedErrorCode());
       }

       THROW_LAST_ERROR_IF_MSG(!result.IsRegistered(), "Package registration failed.");

       if (!context.Args.Contains(Execution::Args::Type::Interactive))
       {
           context.Reporter.Info() << "Successfully Installed! Please launch the application to complete the installation process." << std::endl;
       }
       else
       {
           context.Reporter.Info() << "Successfully Installed! Launching application now. This is necessary to complete the installation." << std::endl;
       }
 
    }

    void LaunchPWA(Execution::Context& context)
    {  
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        winrt::init_apartment();
  
        std::wstring aumid = L"";
        CoInitialize(nullptr);

        for (auto const& package : PackageManager{}.FindPackagesForUser(L"", winrt::to_hstring(manifest.Name), winrt::to_hstring(manifest.Publisher)))
        {
            aumid = package.Id().FamilyName();
            aumid.append(L"!App");
        }

        winrt::com_ptr<IApplicationActivationManager> spAppActivationManager;
        HRESULT hrResult = E_INVALIDARG;

        if (!aumid.empty())
        {
            // Instantiate IApplicationActivationManager
            hrResult = CoCreateInstance(CLSID_ApplicationActivationManager,
                nullptr,
                CLSCTX_LOCAL_SERVER, //This has to be LocalServer, Don't know why cannot be InprocServer
                IID_IApplicationActivationManager,
                (LPVOID*)&spAppActivationManager);

            if (SUCCEEDED(hrResult))
            {
                // This call ensures that the app is launched as the foreground window
                hrResult = CoAllowSetForegroundWindow(spAppActivationManager.get(), nullptr);
                // Launch the app
                if (SUCCEEDED(hrResult))
                {
                    DWORD dwPID = 0;
                    ACTIVATEOPTIONS activate_options = AO_NOERRORUI;
                    hrResult = spAppActivationManager->ActivateApplication(aumid.c_str(),
                        NULL,
                        activate_options
                        ,
                        &dwPID);
                }
            }
        }
    }

 
    void GeneratePwaMsix(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>().value();
        auto& manifest = context.Get<Execution::Data::Manifest>();
        std::string name;
        std::string publisher;
        std::filesystem::path tempInstallerPath = Runtime::GetPathTo(Runtime::PathName::Temp);
        AICLI_LOG(CLI, Info, << "Generated temp download path: " << tempInstallerPath);
        std::filesystem::create_directories(tempInstallerPath);
        tempInstallerPath /= (Utility::ConvertToUTF16(manifest.Id + '.' + manifest.Version + ".msix"));
        
        context.Reporter.ExecuteWithProgress(std::bind(Utility::DownloadPWAInstaller,installer.Url, manifest.Id, manifest.Version, tempInstallerPath, std::ref(publisher), std::ref(name), std::placeholders::_1));
        
        //Update the name and publisher based on most updated version of PWA
        manifest.Name = name;
        manifest.Publisher = publisher;

        context.Add<Execution::Data::InstallerPath>(std::move(tempInstallerPath));

    }
   

    void DownloadInstallerFile(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        const auto& installer = context.Get<Execution::Data::Installer>().value();

        std::filesystem::path tempInstallerPath = Runtime::GetPathTo(Runtime::PathName::Temp);
        tempInstallerPath /= Utility::ConvertToUTF16(manifest.Id + '.' + manifest.Version);

        AICLI_LOG(CLI, Info, << "Generated temp download path: " << tempInstallerPath);

        context.Reporter.Info() << "Downloading " << Execution::UrlEmphasis << installer.Url << std::endl;

        auto hash = context.Reporter.ExecuteWithProgress(std::bind(Utility::Download,
            installer.Url,
            tempInstallerPath,
            std::placeholders::_1,
            true));

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
            if (e.code() == HRESULT_FROM_WIN32(ERROR_NO_RANGES_PROCESSED))
            {
                // Server does not support range request, use download
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
        }
    }

    void ExecuteInstaller(Execution::Context& context)
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
            context << ShellExecuteInstall;
            break;
        case ManifestInstaller::InstallerTypeEnum::Msix:
            context << MsixInstall;
            break;
        case ManifestInstaller::InstallerTypeEnum::MSStore:
            context <<
                EnsureFeatureEnabled(Settings::ExperimentalFeature::Feature::ExperimentalMSStore) <<
                MSStoreInstall;
            break;
        case ManifestInstaller::InstallerTypeEnum::PWA:
            if (!context.Args.Contains(Execution::Args::Type::Interactive))
            {
                context << EnsureFeatureEnabled(Settings::ExperimentalFeature::Feature::ExperimentalPWA) << PWAInstall;
            }
            else
            {
                context << EnsureFeatureEnabled(Settings::ExperimentalFeature::Feature::ExperimentalPWA) << PWAInstall << LaunchPWA;
            }
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
        winrt::Windows::Foundation::Uri uri = nullptr;
        if (context.Contains(Execution::Data::InstallerPath))
        {
            uri = winrt::Windows::Foundation::Uri(context.Get<Execution::Data::InstallerPath>().c_str());
        }
        else
        {
            uri = winrt::Windows::Foundation::Uri(Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->Url));
        }

        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        try
        {
            DeploymentOptions deploymentOptions =
                DeploymentOptions::ForceApplicationShutdown |
                DeploymentOptions::ForceTargetApplicationShutdown;
            context.Reporter.ExecuteWithProgress(std::bind(Deployment::RequestAddPackage, uri, deploymentOptions, std::placeholders::_1));
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
                std::filesystem::remove(path);
        }
    }

    void MSStoreInstall(Execution::Context& context)
    {
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);

        constexpr std::wstring_view s_StoreClientName = L"Microsoft.WindowsStore"sv;
        constexpr std::wstring_view s_StoreClientPublisher = L"CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US"sv;

        // Policy check
        AppInstallManager installManager;
        if (installManager.IsStoreBlockedByPolicyAsync(s_StoreClientName, s_StoreClientPublisher).get())
        {
            context.Reporter.Error() << Resource::String::MSStoreInstallStoreClientBlocked << std::endl;
            AICLI_LOG(CLI, Error, << "Store client is blocked by policy. MSStore install failed.");
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_MSSTORE_BLOCKED_BY_POLICY);
        }

        if (!installManager.GetIsAppAllowedToInstallAsync(productId).get())
        {
            context.Reporter.Error() << Resource::String::MSStoreInstallAppBlocked << std::endl;
            AICLI_LOG(CLI, Error, << "App is blocked by policy. MSStore install failed. ProductId: " << Utility::ConvertToUTF8(productId));
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_MSSTORE_APP_BLOCKED_BY_POLICY);
        }

        // Verifying/Acquiring product ownership
        context.Reporter.Info() << Resource::String::MSStoreInstallTryGetEntitlement << std::endl;
        GetEntitlementResult enr = installManager.GetFreeUserEntitlementAsync(productId, winrt::hstring(), winrt::hstring()).get();

        if (enr.Status() == GetEntitlementStatus::Succeeded)
        {
            context.Reporter.Info() << Resource::String::MSStoreInstallGetEntitlementSuccess << std::endl;
            AICLI_LOG(CLI, Error, << "Get entitlement succeeded.");
        }
        else
        {
            if (enr.Status() == GetEntitlementStatus::NoStoreAccount)
            {
                context.Reporter.Info() << Resource::String::MSStoreInstallGetEntitlementNoStoreAccount << std::endl;
                AICLI_LOG(CLI, Error, << "Get entitlement failed. No Store account.");
            }
            else if (enr.Status() == GetEntitlementStatus::NetworkError)
            {
                context.Reporter.Info() << Resource::String::MSStoreInstallGetEntitlementNetworkError << std::endl;
                AICLI_LOG(CLI, Error, << "Get entitlement failed. Network error.");
            }
            else if (enr.Status() == GetEntitlementStatus::ServerError)
            {
                context.Reporter.Info() << Resource::String::MSStoreInstallGetEntitlementServerError << std::endl;
                AICLI_LOG(CLI, Error, << "Get entitlement succeeded. Server error. ProductId: " << Utility::ConvertToUTF8(productId));
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_MSSTORE_INSTALL_FAILED);
        }

        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        IVectorView<AppInstallItem> installItems = installManager.StartProductInstallAsync(
            productId,              // ProductId
            winrt::hstring(),       // CatalogId
            winrt::hstring(),       // FlightId
            L"WinGetCli",           // ClientId
            false,                  // repair
            false,
            winrt::hstring(),
            nullptr).get();

        for (auto const& installItem : installItems)
        {
            AICLI_LOG(CLI, Info, <<
                "Started MSStore package installation. ProductId: " << Utility::ConvertToUTF8(installItem.ProductId()) <<
                " PackageFamilyName: " << Utility::ConvertToUTF8(installItem.PackageFamilyName()));
        }

        HRESULT errorCode = S_OK;
        context.Reporter.ExecuteWithProgress(
            [&](IProgressCallback& progress)
            {
                // We are aggregating all AppInstallItem progresses into one.
                // Averaging every progress for now until we have a better way to find overall progress.
                uint64_t overallProgressMax = 100 * installItems.Size();
                uint64_t currentProgress = 0;

                while (currentProgress < overallProgressMax)
                {
                    currentProgress = 0;

                    for (auto const& installItem : installItems)
                    {
                        const auto& status = installItem.GetCurrentStatus();
                        currentProgress += static_cast<uint64_t>(status.PercentComplete());

                        errorCode = status.ErrorCode();

                        if (!SUCCEEDED(errorCode))
                        {
                            return;
                        }
                    }

                    // It may take a while for Store client to pick up the install request.
                    // So we show indefinite progress here to avoid a progress bar stuck at 0.
                    if (currentProgress > 0)
                    {
                        progress.OnProgress(currentProgress, overallProgressMax, ProgressType::Percent);
                    }

                    if (progress.IsCancelled())
                    {
                        for (auto const& installItem : installItems)
                        {
                            installItem.Cancel();
                        }
                    }

                    Sleep(100);
                }
            });

        if (SUCCEEDED(errorCode))
        {
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
        else
        {
            context.Reporter.Info() << Resource::String::MSStoreInstallFailed << ' ' << WINGET_OSTREAM_FORMAT_HRESULT(errorCode) << std::endl;
            AICLI_LOG(CLI, Error, << "MSStore install failed. ProductId: " << Utility::ConvertToUTF8(productId) << " HResult: " << WINGET_OSTREAM_FORMAT_HRESULT(errorCode));
            AICLI_TERMINATE_CONTEXT(errorCode);
        }
    }
}
