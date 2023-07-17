// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MSStoreInstallerHandler.h"
#include <winget/MSStore.h>
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
