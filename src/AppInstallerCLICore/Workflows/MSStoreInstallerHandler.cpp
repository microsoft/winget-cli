// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MSStoreInstallerHandler.h"
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

        const auto& installer = context.Get<Execution::Data::Installer>().value();
            std::string storeRestEndpoint = MSStore::GetMSStoreCatalogRestApi(installer.ProductId, installer.Locale);

            AppInstaller::Http::HttpClientHelper httpClientHelper;
            std::optional<web::json::value> jsonObject = httpClientHelper.HandleGet(JSON::GetUtilityString(storeRestEndpoint));

            if (!jsonObject)
            {
                AICLI_LOG(Core, Error, << "No json object found");
            }

            const auto& packages = MSStore::DeserializeMSStoreCatalogPackages(jsonObject.value());

            // Language
            std::vector<std::string> requiredLocale;
            if (context.Args.Contains(Execution::Args::Type::Locale))
            {
                requiredLocale.emplace_back(context.Args.GetArg(Execution::Args::Type::Locale));
            }

            // Architectures
            std::vector<Utility::Architecture> allowedArchitectures;
            if (context.Contains(Execution::Data::AllowedArchitectures))
            {
                // Com caller can directly set allowed architectures
                allowedArchitectures = context.Get<Execution::Data::AllowedArchitectures>();
            }
            else if (context.Args.Contains(Execution::Args::Type::InstallArchitecture))
            {
                allowedArchitectures.emplace_back(Utility::ConvertToArchitectureEnum(context.Args.GetArg(Execution::Args::Type::InstallArchitecture)));
            }

            DisplayCatalogPackageComparator packageComparator(requiredLocale, allowedArchitectures);
            auto result = packageComparator.GetPreferredPackage(packages);

            if (!result)
            {
                context.Reporter.Error() << Resource::String::MSStoreDownloadPackageNotFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_MSSTORE_DOWNLOAD_NO_APPLICABLE_PACKAGE);
            }

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
