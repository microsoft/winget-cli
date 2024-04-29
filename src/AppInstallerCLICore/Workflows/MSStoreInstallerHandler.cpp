// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MSStoreInstallerHandler.h"
#include "WorkflowBase.h"
#include <winget/MSStore.h>
#include <winget/MSStoreDownload.h>
#include <winget/SelfManagement.h>

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

        HRESULT DownloadMSStorePackageFile()
        {
            
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
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::StoreDownload))
        {
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }

        // YAO: Info for --rename not working formsstore download
        // YAO: Info for authentication

        const auto& installer = context.Get<Execution::Data::Installer>().value();

        Utility::Architecture requiredArchitecture = Utility::Architecture::Unknown;
        std::string requiredLocale;
        if (context.Args.Contains(Execution::Args::Type::InstallArchitecture))
        {
            requiredArchitecture = Utility::ConvertToArchitectureEnum(context.Args.GetArg(Execution::Args::Type::InstallArchitecture));
        }
        if (context.Args.Contains(Execution::Args::Type::Locale))
        {
            requiredLocale = context.Args.GetArg(Execution::Args::Type::Locale);
        }

        MSStoreDownloadContext downloadContext{ installer.ProductId, requiredArchitecture, requiredLocale, GetAuthenticationArguments(context) };

        MSStoreDownloadInfo downloadInfo;
        try
        {
            // YAO: info reporting
            downloadInfo = downloadContext.GetDwonloadInfo();

            if (downloadInfo.MainPackages.empty())
            {
                context.Reporter.Error() << Resource::String::MSStoreDownloadPackageNotFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_DISPLAYCATALOG_PACKAGE);
            }
        }
        catch (const wil::ResultException& re)
        {
            // YAO: error reporting
            AICLI_TERMINATE_CONTEXT(re.GetErrorCode());
        }

        bool skipDependencies = context.Args.Contains(Execution::Args::Type::SkipDependencies);

        // Prepare directories
        THROW_HR_IF(E_UNEXPECTED, !context.Contains(Execution::Data::DownloadDirectory));
        std::filesystem::path downloadDirectory = context.Get<Execution::Data::DownloadDirectory>();
        std::filesystem::path dependenciesDirectory = downloadDirectory / L"Dependencies";

        // Create directories if needed.
        auto directoryToCreate = skipDependencies ? downloadDirectory : dependenciesDirectory;
        if (!std::filesystem::exists(directoryToCreate))
        {
            std::filesystem::create_directories(directoryToCreate);
        }
        else
        {
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE), !std::filesystem::is_directory(directoryToCreate));
        }

        if (!skipDependencies)
        {
            // YAO: info reporting
            for (auto const& dependencyPackage : downloadInfo.DependencyPackages)
            {
                HRESULT hr = DownloadMSStorePackageFile();
                if (FAILED(hr))
                {

                }
            }
        }

        for (auto const& dependencyPackage : downloadInfo.MainPackages)
        {
            // YAO: info reporting
            HRESULT hr = DownloadMSStorePackageFile();
            if (FAILED(hr))
            {

            }
        }

        if (!context.Args.Contains(Execution::Args::Type::SkipMicrosoftStorePackageLicense))
        {
            // YAO: info reporting
            std::vector<BYTE> licenseContent;
            try
            {
                licenseContent = downloadContext.GetLicense();
            }
            catch (const wil::ResultException& re)
            {
                // YAO: error reporting
                AICLI_TERMINATE_CONTEXT(re.GetErrorCode());
            }

            THROW_HR_IF(E_UNEXPECTED, licenseContent.empty());
            std::filesystem::path licenseFilePath = downloadDirectory / Utility::ConvertToUTF16(installer.ProductId + "_License.xml");
            std::ofstream licenseFile(licenseFilePath, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
            licenseFile.write((const char *)&licenseContent[0], licenseContent.size());
            licenseFile.flush();
            licenseFile.close();

            // YAO: info reporting success
        }

        std::string storeRestEndpoint = MSStore::GetMSStoreCatalogRestApi(installer.ProductId, installer.Locale);

            AppInstaller::Http::HttpClientHelper httpClientHelper;
            std::optional<web::json::value> jsonObject = httpClientHelper.HandleGet(JSON::GetUtilityString(storeRestEndpoint));

            if (!jsonObject)
            {
                AICLI_LOG(Core, Error, << "No json object found");
            }

            const auto& packages = MSStore::DeserializeMSStoreCatalogPackages(jsonObject.value());

            DisplayCatalogPackageComparator packageComparator(requiredLocale, allowedArchitectures);
            auto result = packageComparator.GetPreferredPackage(packages);

            

            auto preferredPackage = result.value();

            AICLI_LOG(Core, Info, << "WuCategoryId: " << preferredPackage.WuCategoryId);
            AICLI_LOG(Core, Info, << "ContentId: " << preferredPackage.ContentId);

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
