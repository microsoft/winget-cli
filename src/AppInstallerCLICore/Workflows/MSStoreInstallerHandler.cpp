// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MSStoreInstallerHandler.h"
#include "WorkflowBase.h"
#include <AppInstallerSHA256.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerRuntime.h>
#include <winget/Filesystem.h>
#include <winget/MSStore.h>
#include <winget/MSStoreDownload.h>
#include <winget/SelfManagement.h>

namespace AppInstaller::CLI::Workflow
{
    void DownloadInstallerFile(Execution::Context& context);
}

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::MSStore;
    using namespace AppInstaller::SelfManagement;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::ApplicationModel::Store::Preview::InstallControl;

    namespace
    {
        Utility::LocIndString GetErrorCodeString(const HRESULT errorCode)
        {
            std::ostringstream ssError;
            ssError << WINGET_OSTREAM_FORMAT_HRESULT(errorCode);
            return Utility::LocIndString{ ssError.str() };
        }

        HRESULT EnsureStorePolicySatisfiedImpl(const std::wstring& productId, bool bypassPolicy)
        {
            constexpr std::wstring_view s_StoreClientName = L"Microsoft.WindowsStore"sv;
            constexpr std::wstring_view s_StoreClientPublisher = L"CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US"sv;

            // Policy check
            AppInstallManager installManager;

            if (!bypassPolicy && installManager.IsStoreBlockedByPolicyAsync(s_StoreClientName, s_StoreClientPublisher).get())
            {
                AICLI_LOG(CLI, Error, << "Store client is blocked by policy. MSStore execution failed.");
                return APPINSTALLER_CLI_ERROR_MSSTORE_BLOCKED_BY_POLICY;
            }

            if (!installManager.GetIsAppAllowedToInstallAsync(productId).get())
            {
                AICLI_LOG(CLI, Error, << "App is blocked by policy. MSStore execution failed. ProductId: " << Utility::ConvertToUTF8(productId));
                return APPINSTALLER_CLI_ERROR_MSSTORE_APP_BLOCKED_BY_POLICY;
            }

            return S_OK;
        }

        void AppInstallerUpdate(bool preferStub, bool bypassPolicy, Execution::Context& context)
        {
            auto appInstId = std::wstring{ s_AppInstallerProductId };
            THROW_IF_FAILED(EnsureStorePolicySatisfiedImpl(appInstId, bypassPolicy));
            SetStubPreferred(preferStub);

            auto installOperation = MSStoreOperation(MSStoreOperationType::Update, appInstId, Manifest::ScopeEnum::User, true, true);

            HRESULT hr = S_OK;
            context.Reporter.ExecuteWithProgress(
                [&](IProgressCallback& progress)
                {
                    hr = installOperation.StartAndWaitForOperation(progress);
                });

            THROW_IF_FAILED(hr);
        }

        HRESULT DownloadMSStorePackageFile(const MSStore::MSStoreDownloadFile& downloadFile, const std::filesystem::path& downloadDirectory, Execution::Context& context)
        {
            try
            {
                // Create a sub context to execute the package download
                auto subContextPtr = context.CreateSubContext();
                Execution::Context& subContext = *subContextPtr;
                auto previousThreadGlobals = subContext.SetForCurrentThread();

                // Populate Installer and temp download path for sub context
                Manifest::ManifestInstaller installer;
                installer.Url = downloadFile.Url;
                installer.Sha256 = downloadFile.Sha256;
                subContext.Add<Execution::Data::Installer>(std::move(installer));

                auto tempInstallerPath = Runtime::GetPathTo(Runtime::PathName::Temp);
                tempInstallerPath /= Utility::SHA256::ConvertToString(downloadFile.Sha256);
                AICLI_LOG(CLI, Info, << "Generated temp download path: " << tempInstallerPath);
                subContext.Add<Execution::Data::InstallerPath>(tempInstallerPath);

                subContext << Workflow::DownloadInstallerFile;
                if (subContext.IsTerminated())
                {
                    RETURN_HR(subContext.GetTerminationHR());
                }

                // Verify hash
                const auto& hashPair = subContext.Get<Execution::Data::DownloadHashInfo>();
                if (std::equal(hashPair.first.begin(), hashPair.first.end(), hashPair.second.Sha256Hash.begin()))
                {
                    AICLI_LOG(CLI, Info, << "Microsoft Store package hash verified");
                    subContext.Reporter.Info() << Resource::String::MSStoreDownloadPackageHashVerified << std::endl;
                    // Trust direct download from Store if hash matched
                    Utility::ApplyMotwIfApplicable(tempInstallerPath, URLZONE_TRUSTED);
                }
                else
                {
                    if (!subContext.Args.Contains(Execution::Args::Type::HashOverride))
                    {
                        AICLI_LOG(CLI, Error, << "Microsoft Store package hash mismatch");
                        subContext.Reporter.Error() << Resource::String::MSStoreDownloadPackageHashMismatch << std::endl;
                        RETURN_HR(APPINSTALLER_CLI_ERROR_INSTALLER_HASH_MISMATCH);
                    }
                    else
                    {
                        AICLI_LOG(CLI, Warning, << "Microsoft Store package hash mismatch, but overridden.");
                        subContext.Reporter.Warn() << Resource::String::MSStoreDownloadPackageHashMismatch << std::endl;
                    }
                }

                auto renamedDownloadedPackage = downloadDirectory / Utility::ConvertToUTF16(downloadFile.FileName);
                Filesystem::RenameFile(tempInstallerPath, renamedDownloadedPackage);
                subContext.Reporter.Info() << Resource::String::MSStoreDownloadPackageDownloaded(Utility::LocIndView{ renamedDownloadedPackage.u8string() }) << std::endl;

                return S_OK;
            }
            catch (...)
            {
                AICLI_LOG(CLI, Error, << "Microsoft Store package download failed. File: " << downloadFile.FileName);
                context.Reporter.Error() << Resource::String::MSStoreDownloadPackageDownloadFailed(Utility::LocIndView{ downloadFile.FileName }) << std::endl;
                RETURN_HR(APPINSTALLER_CLI_ERROR_DOWNLOAD_FAILED);
            }
        }
    }

    void MSStoreInstall(Execution::Context& context)
    {
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);
        auto scope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        bool isSilentMode = context.Args.Contains(Execution::Args::Type::Silent);
        bool force = context.Args.Contains(Execution::Args::Type::Force);

        auto installOperation = MSStoreOperation(MSStoreOperationType::Install, productId, scope, isSilentMode, force);

        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        HRESULT hr = S_OK;
        context.Reporter.ExecuteWithProgress(
            [&](IProgressCallback& progress)
            {
                hr = installOperation.StartAndWaitForOperation(progress);
            });

        if (SUCCEEDED(hr))
        {
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
        else
        {
            if (hr == APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED)
            {
                context.Reporter.Error() << Resource::String::InstallFlowReturnCodeSystemNotSupported << std::endl;
                context.Add<Execution::Data::OperationReturnCode>(static_cast<DWORD>(APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED));
            }
            else
            {
                auto errorCodeString = GetErrorCodeString(hr);
                context.Reporter.Error() << Resource::String::MSStoreInstallOrUpdateFailed(errorCodeString) << std::endl;
                context.Add<Execution::Data::OperationReturnCode>(hr);
                AICLI_LOG(CLI, Error, << "MSStore install failed. ProductId: " << Utility::ConvertToUTF8(productId) << " HResult: " << errorCodeString);
            }

            AICLI_TERMINATE_CONTEXT(hr);
        }
    }

    void MSStoreUpdate(Execution::Context& context)
    {
        bool isSilentMode = context.Args.Contains(Execution::Args::Type::Silent);
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);
        auto scope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        bool force = context.Args.Contains(Execution::Args::Type::Force);

        auto installOperation = MSStoreOperation(MSStoreOperationType::Update, productId, scope, isSilentMode, force);

        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        HRESULT hr = S_OK;
        context.Reporter.ExecuteWithProgress(
            [&](IProgressCallback& progress)
            {
                hr = installOperation.StartAndWaitForOperation(progress);
            });

        if (SUCCEEDED(hr))
        {
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
        else
        {
            if (hr == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE)
            {
                context.Reporter.Info() << Resource::String::UpdateNotApplicable << std::endl
                    << Resource::String::UpdateNotApplicableReason << std::endl;
            }
            else
            {
                auto errorCodeString = GetErrorCodeString(hr);
                context.Reporter.Error() << Resource::String::MSStoreInstallOrUpdateFailed(errorCodeString) << std::endl;
                context.Add<Execution::Data::OperationReturnCode>(hr);
                AICLI_LOG(CLI, Error, << "MSStore execution failed. ProductId: " << Utility::ConvertToUTF8(productId) << " HResult: " << errorCodeString);
            }

            AICLI_TERMINATE_CONTEXT(hr);
        }
    }

    void MSStoreRepair(Execution::Context& context)
    {
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);
        auto scope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        bool isSilentMode = context.Args.Contains(Execution::Args::Type::Silent);
        bool force = context.Args.Contains(Execution::Args::Type::Force);

        auto repairOperation = MSStoreOperation(MSStoreOperationType::Repair, productId, scope, isSilentMode, force);

        context.Reporter.Info() << Resource::String::RepairFlowStartingPackageRepair << std::endl;

        HRESULT hr = S_OK;
        context.Reporter.ExecuteWithProgress(
            [&](IProgressCallback& progress)
            {
                hr = repairOperation.StartAndWaitForOperation(progress);
            });

        if (SUCCEEDED(hr))
        {
            context.Reporter.Info() << Resource::String::RepairFlowRepairSuccess << std::endl;
        }
        else
        {
            if (hr == APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED)
            {
                context.Reporter.Error() << Resource::String::InstallFlowReturnCodeSystemNotSupported << std::endl;
                context.Add<Execution::Data::OperationReturnCode>(static_cast<DWORD>(APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED));
            }
            else
            {
                auto errorCodeString = GetErrorCodeString(hr);
                context.Reporter.Error() << Resource::String::MSStoreRepairFailed(errorCodeString) << std::endl;
                context.Add<Execution::Data::OperationReturnCode>(hr);
                AICLI_LOG(CLI, Error, << "MSStore repair failed. ProductId: " << Utility::ConvertToUTF8(productId) << " HResult: " << errorCodeString);
            }

            AICLI_TERMINATE_CONTEXT(hr);
        }
    }

    void MSStoreDownload(Execution::Context& context)
    {
        if (context.Args.Contains(Execution::Args::Type::Rename))
        {
            context.Reporter.Warn() << Resource::String::MSStoreDownloadRenameNotSupported << std::endl;
        }

        // Authentication notice
        context.Reporter.Warn() << Resource::String::MSStoreDownloadAuthenticationNotice << std::endl;
        context.Reporter.Warn() << Resource::String::MSStoreDownloadMultiplePackagesNotice << std::endl;

        const auto& installer = context.Get<Execution::Data::Installer>().value();

        Utility::Architecture requiredArchitecture = Utility::Architecture::Unknown;
        Manifest::PlatformEnum requiredPlatform = Manifest::PlatformEnum::Unknown;
        std::string requiredLocale;
        if (context.Args.Contains(Execution::Args::Type::InstallerArchitecture))
        {
            requiredArchitecture = Utility::ConvertToArchitectureEnum(context.Args.GetArg(Execution::Args::Type::InstallerArchitecture));
        }
        if (context.Args.Contains(Execution::Args::Type::Platform))
        {
            requiredPlatform = Manifest::ConvertToPlatformEnumForMSStoreDownload(context.Args.GetArg(Execution::Args::Type::Platform));
        }
        if (context.Args.Contains(Execution::Args::Type::Locale))
        {
            requiredLocale = context.Args.GetArg(Execution::Args::Type::Locale);
        }

        MSStoreDownloadContext downloadContext{ installer.ProductId, requiredArchitecture, requiredPlatform, requiredLocale, GetAuthenticationArguments(context) };

        MSStoreDownloadInfo downloadInfo;
        try
        {
            context.Reporter.Info() << Resource::String::MSStoreDownloadGetDownloadInfo << std::endl;

            downloadInfo = downloadContext.GetDownloadInfo();
        }
        catch (const wil::ResultException& re)
        {
            AICLI_LOG(CLI, Error, << "Getting MSStore package download info failed. Error code: " << re.GetErrorCode());

            switch (re.GetErrorCode())
            {
            case APPINSTALLER_CLI_ERROR_NO_APPLICABLE_DISPLAYCATALOG_PACKAGE:
            case APPINSTALLER_CLI_ERROR_NO_APPLICABLE_SFSCLIENT_PACKAGE:
                context.Reporter.Error() << Resource::String::MSStoreDownloadNoApplicablePackageFound << std::endl;
                break;
            case APPINSTALLER_CLI_ERROR_SFSCLIENT_PACKAGE_NOT_SUPPORTED:
                context.Reporter.Error() << Resource::String::MSStoreDownloadPackageDownloadNotSupported << std::endl;
                break;
            default:
                context.Reporter.Error() << Resource::String::MSStoreDownloadGetDownloadInfoFailed << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(re.GetErrorCode());
        }

        bool skipDependencies = context.Args.Contains(Execution::Args::Type::SkipDependencies);

        // Prepare directories
        std::filesystem::path downloadDirectory = context.Get<Execution::Data::DownloadDirectory>();
        std::filesystem::path dependenciesDirectory = downloadDirectory / L"Dependencies";

        // Create directories if needed.
        auto directoryToCreate = (skipDependencies || downloadInfo.DependencyPackages.empty()) ? downloadDirectory : dependenciesDirectory;
        if (!std::filesystem::exists(directoryToCreate))
        {
            std::filesystem::create_directories(directoryToCreate);
        }
        else
        {
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE), !std::filesystem::is_directory(directoryToCreate));
        }

        // Download dependency packages
        if (!skipDependencies)
        {
            AICLI_LOG(CLI, Info, << "Downloading MSStore dependency packages");
            context.Reporter.Info() << Resource::String::MSStoreDownloadDependencyPackages << std::endl;

            for (auto const& dependencyPackage : downloadInfo.DependencyPackages)
            {
                THROW_IF_FAILED(DownloadMSStorePackageFile(dependencyPackage, dependenciesDirectory, context));
            }
        }

        // Download main packages
        AICLI_LOG(CLI, Info, << "Downloading MSStore main packages");
        context.Reporter.Info() << Resource::String::MSStoreDownloadMainPackages << std::endl;
        for (auto const& mainPackage : downloadInfo.MainPackages)
        {
            THROW_IF_FAILED(DownloadMSStorePackageFile(mainPackage, downloadDirectory, context));
        }

        context.Reporter.Info() << Resource::String::MSStoreDownloadPackageDownloadSuccess << std::endl;

        // Get license
        if (!context.Args.Contains(Execution::Args::Type::SkipMicrosoftStorePackageLicense))
        {
            AICLI_LOG(CLI, Info, << "Getting MSStore package license");
            context.Reporter.Info() << Resource::String::MSStoreDownloadGetLicense << std::endl;

            std::vector<BYTE> licenseContent;
            try
            {
                licenseContent = downloadContext.GetLicense(downloadInfo.ContentId);
            }
            catch (const wil::ResultException& re)
            {
                if (re.GetErrorCode() == APPINSTALLER_CLI_ERROR_LICENSING_API_FAILED_FORBIDDEN)
                {
                    AICLI_LOG(CLI, Warning, << "Getting MSStore package license failed. The Microsoft Entra Id account does not have privilege.");
                    context.Reporter.Warn() << Resource::String::MSStoreDownloadGetLicenseForbidden << std::endl;
                }
                else
                {
                    AICLI_LOG(CLI, Warning, << "Getting MSStore package license failed. Error code: " << re.GetErrorCode());
                    context.Reporter.Warn() << Resource::String::MSStoreDownloadGetLicenseFailed << std::endl;
                }

                AICLI_TERMINATE_CONTEXT(re.GetErrorCode());
            }

            std::filesystem::path licenseFilePath = downloadDirectory / Utility::ConvertToUTF16(installer.ProductId + "_License.xml");
            std::ofstream licenseFile(licenseFilePath, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
            licenseFile.write((const char *)&licenseContent[0], licenseContent.size());
            licenseFile.flush();
            licenseFile.close();

            AICLI_LOG(CLI, Info, << "Getting MSStore package license success");
            context.Reporter.Info() << Resource::String::MSStoreDownloadGetLicenseSuccess(Utility::LocIndView{ licenseFilePath.u8string() }) << std::endl;
        }
    }

    void EnsureStorePolicySatisfied(Execution::Context& context)
    {
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);
        bool bypassStorePolicy = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::BypassIsStoreClientBlockedPolicyCheck);

        HRESULT hr = EnsureStorePolicySatisfiedImpl(productId, bypassStorePolicy);
        if (FAILED(hr))
        {
            if (hr == APPINSTALLER_CLI_ERROR_MSSTORE_BLOCKED_BY_POLICY)
            {
                context.Reporter.Error() << Resource::String::MSStoreStoreClientBlocked << std::endl;
            }
            else if (hr == APPINSTALLER_CLI_ERROR_MSSTORE_APP_BLOCKED_BY_POLICY)
            {
                context.Reporter.Error() << Resource::String::MSStoreAppBlocked << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(hr);
        }
    }

    void EnableConfiguration(Execution::Context& context)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        AppInstallerUpdate(false, true, context);
#else
        if (IsStubPackage())
        {
            context.Reporter.Info() << Resource::String::ConfigurationEnablingMessage << std::endl;
            bool bypassStorePolicy = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::BypassIsStoreClientBlockedPolicyCheck);
            AppInstallerUpdate(false, bypassStorePolicy, context);
        }
        else
        {
            context.Reporter.Info() << Resource::String::ConfigurationEnabledMessage << std::endl;
        }
#endif
    }

    void DisableConfiguration(Execution::Context& context)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        AppInstallerUpdate(true, true, context);
#else
        if (!IsStubPackage())
        {
            context.Reporter.Info() << Resource::String::ConfigurationDisablingMessage << std::endl;
            bool bypassStorePolicy = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::BypassIsStoreClientBlockedPolicyCheck);
            AppInstallerUpdate(true, bypassStorePolicy, context);
        }
        else
        {
            context.Reporter.Info() << Resource::String::ConfigurationDisabledMessage << std::endl;
        }
#endif
    }
}
