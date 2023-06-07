// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MSStoreInstallerHandler.h"
#include <winget/ManifestCommon.h>
#include <winget/Runtime.h>
#include <winget/SelfManagement.h>

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::SelfManagement;
    using namespace std::string_view_literals;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::ApplicationModel::Store::Preview::InstallControl;

    static constexpr std::wstring_view s_AppInstallerProductId = L"9NBLGGH4NNS1"sv;

    namespace
    {
        HRESULT WaitForMSStoreOperation(Execution::Context& context, IVectorView<AppInstallItem>& installItems)
        {
            bool isSilentMode = context.Args.Contains(Execution::Args::Type::Silent);

            for (auto const& installItem : installItems)
            {
                AICLI_LOG(CLI, Info, <<
                    "Started MSStore package execution. ProductId: " << Utility::ConvertToUTF8(installItem.ProductId()) <<
                    " PackageFamilyName: " << Utility::ConvertToUTF8(installItem.PackageFamilyName()));

                if (isSilentMode)
                {
                    installItem.InstallInProgressToastNotificationMode(AppInstallationToastNotificationMode::NoToast);
                    installItem.CompletedInstallToastNotificationMode(AppInstallationToastNotificationMode::NoToast);
                }
            }

            HRESULT errorCode = S_OK;
            context.Reporter.ExecuteWithProgress(
                [&](IProgressCallback& progress)
                {
                    // We are aggregating all AppInstallItem progresses into one.
                    // Averaging every progress for now until we have a better way to find overall progress.
                    uint64_t overallProgressMax = 100 * static_cast<uint64_t>(installItems.Size());
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

            return errorCode;
        }

        // The type of entitlement we were able to acquire/ensure.
        enum class EntitlementType
        {
            None,
            User,
            Device,
        };

        EntitlementType EnsureFreeEntitlement(Execution::Context& context, const std::wstring& productId, Manifest::ScopeEnum scope)
        {
            AppInstallManager installManager;

            AICLI_LOG(CLI, Info, << "Getting entitlement for ProductId: " << Utility::ConvertToUTF8(productId));

            // Verifying/Acquiring product ownership
            context.Reporter.Info() << Resource::String::MSStoreInstallTryGetEntitlement << std::endl;

            GetEntitlementResult entitlementResult{ nullptr };
            EntitlementType result = EntitlementType::None;

            if (scope == Manifest::ScopeEnum::Machine)
            {
                AICLI_LOG(CLI, Info, << "Get device entitlement (machine scope install).");
                result = EntitlementType::Device;
                try
                {
                    entitlementResult = installManager.GetFreeDeviceEntitlementAsync(productId, winrt::hstring(), winrt::hstring()).get();
                }
                CATCH_LOG();
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Get user entitlement.");
                result = EntitlementType::User;
                try
                {
                    entitlementResult = installManager.GetFreeUserEntitlementAsync(productId, winrt::hstring(), winrt::hstring()).get();
                }
                CATCH_LOG();

                if (!entitlementResult || entitlementResult.Status() == GetEntitlementStatus::NoStoreAccount)
                {
                    AICLI_LOG(CLI, Info, << "Get device entitlement (no store account).");
                    result = EntitlementType::Device;
                    try
                    {
                        entitlementResult = installManager.GetFreeDeviceEntitlementAsync(productId, winrt::hstring(), winrt::hstring()).get();
                    }
                    CATCH_LOG();
                }
            }

            if (entitlementResult && entitlementResult.Status() == GetEntitlementStatus::Succeeded)
            {
                AICLI_LOG(CLI, Info, << "Get entitlement succeeded.");
            }
            else if (entitlementResult)
            {
                result = EntitlementType::None;

                if (entitlementResult.Status() == GetEntitlementStatus::NetworkError)
                {
                    AICLI_LOG(CLI, Error, << "Get entitlement failed. Network error.");
                }
                else if (entitlementResult.Status() == GetEntitlementStatus::ServerError)
                {
                    AICLI_LOG(CLI, Error, << "Get entitlement failed. Server error.");
                }
                else
                {
                    AICLI_LOG(CLI, Error, << "Get entitlement failed. Unknown status: " << static_cast<int32_t>(entitlementResult.Status()));
                }
            }
            else
            {
                result = EntitlementType::None;
                AICLI_LOG(CLI, Error, << "Get entitlement failed. Exception.");
            }

            return result;
        }

        Utility::LocIndString GetErrorCodeString(const HRESULT errorCode)
        {
            std::ostringstream ssError;
            ssError << WINGET_OSTREAM_FORMAT_HRESULT(errorCode);
            return Utility::LocIndString{ ssError.str() };
        }

        HRESULT MSStoreUpdateImpl(Execution::Context& context, const std::wstring& productId, Manifest::ScopeEnum scope, bool force)
        {
            // Best effort verifying/acquiring product ownership.
            std::ignore = EnsureFreeEntitlement(context, productId, scope);

            AppInstallManager installManager;
            AppUpdateOptions updateOptions;
            updateOptions.AllowForcedAppRestart(force);

            context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

            // SearchForUpdateAsync will automatically trigger update if found.
            AppInstallItem installItem =  installManager.SearchForUpdatesAsync(
                productId,          // ProductId
                winrt::hstring(),   // SkuId
                winrt::hstring(),
                winrt::hstring(),   // ClientId
                updateOptions
            ).get();

            if (!installItem)
            {
                context.Reporter.Info() << Resource::String::UpdateNotApplicable << std::endl
                    << Resource::String::UpdateNotApplicableReason << std::endl;
                return APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE;
            }

            std::vector<AppInstallItem> installItemVector{ installItem };
            IVectorView<AppInstallItem> installItems = winrt::single_threaded_vector(std::move(installItemVector)).GetView();

            HRESULT errorCode = WaitForMSStoreOperation(context, installItems);

            if (SUCCEEDED(errorCode))
            {
                context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
            }
            else
            {
                auto errorCodeString = GetErrorCodeString(errorCode);
                context.Reporter.Info() << Resource::String::MSStoreInstallOrUpdateFailed(errorCodeString) << std::endl;
                context.Add<Execution::Data::OperationReturnCode>(errorCode);
                AICLI_LOG(CLI, Error, << "MSStore execution failed. ProductId: " << Utility::ConvertToUTF8(productId) << " HResult: " << errorCodeString);
                return errorCode;
            }

            return S_OK;
        }

        HRESULT EnsureStorePolicySatisfiedImpl(Execution::Context& context, const std::wstring& productId)
        {
            constexpr std::wstring_view s_StoreClientName = L"Microsoft.WindowsStore"sv;
            constexpr std::wstring_view s_StoreClientPublisher = L"CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US"sv;

            // Policy check
            AppInstallManager installManager;

            if (!WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::BypassIsStoreClientBlockedPolicyCheck) && installManager.IsStoreBlockedByPolicyAsync(s_StoreClientName, s_StoreClientPublisher).get())
            {
                context.Reporter.Error() << Resource::String::MSStoreStoreClientBlocked << std::endl;
                AICLI_LOG(CLI, Error, << "Store client is blocked by policy. MSStore execution failed.");
                return APPINSTALLER_CLI_ERROR_MSSTORE_BLOCKED_BY_POLICY;
            }

            //if (!installManager.GetIsAppAllowedToInstallAsync(productId).get())
            if (true)
            {
                context.Reporter.Error() << Resource::String::MSStoreAppBlocked << std::endl;
                AICLI_LOG(CLI, Error, << "App is blocked by policy. MSStore execution failed. ProductId: " << Utility::ConvertToUTF8(productId));
                return APPINSTALLER_CLI_ERROR_MSSTORE_APP_BLOCKED_BY_POLICY;
            }

            return S_OK;
        }

        HRESULT AppInstallerUpdate(Execution::Context& context, bool preferStub)
        {
            auto appInstId = std::wstring{ s_AppInstallerProductId };
            RETURN_IF_FAILED(EnsureStorePolicySatisfiedImpl(context, appInstId));
            SetStubPreferred(preferStub);
            RETURN_IF_FAILED(MSStoreUpdateImpl(context, appInstId, Manifest::ScopeEnum::User, true));
            return S_OK;
        }
    }

    void MSStoreInstall(Execution::Context& context)
    {
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);
        auto scope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));

        // Best effort verifying/acquiring product ownership.
        std::ignore = EnsureFreeEntitlement(context, productId, scope);

        AppInstallManager installManager;
        AppInstallOptions installOptions;

        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        if (context.Args.Contains(Execution::Args::Type::Silent))
        {
            installOptions.InstallInProgressToastNotificationMode(AppInstallationToastNotificationMode::NoToast);
            installOptions.CompletedInstallToastNotificationMode(AppInstallationToastNotificationMode::NoToast);
        }

        if (Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope)) == Manifest::ScopeEnum::Machine)
        {
            // TODO: There was a bug in InstallService where admin user is incorrectly identified as not admin,
            // causing false access denied on many OS versions.
            // Remove this check when the OS bug is fixed and back ported.
            if (!Runtime::IsRunningAsSystem())
            {
                context.Reporter.Error() << Resource::String::InstallFlowReturnCodeSystemNotSupported << std::endl;
                context.Add<Execution::Data::OperationReturnCode>(static_cast<DWORD>(APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED));
                AICLI_LOG(CLI, Error, << "Device wide install for msstore type is not supported under admin context.");
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED);
            }

            installOptions.InstallForAllUsers(true);
        }

        IVectorView<AppInstallItem> installItems = installManager.StartProductInstallAsync(
            productId,              // ProductId
            winrt::hstring(),       // FlightId
            L"WinGetCli",           // ClientId
            winrt::hstring(),
            installOptions).get();

        HRESULT errorCode = WaitForMSStoreOperation(context, installItems);

        if (SUCCEEDED(errorCode))
        {
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
        else
        {
            auto errorCodeString = GetErrorCodeString(errorCode);
            context.Reporter.Error() << Resource::String::MSStoreInstallOrUpdateFailed(errorCodeString) << std::endl;
            context.Add<Execution::Data::OperationReturnCode>(errorCode);
            AICLI_LOG(CLI, Error, << "MSStore install failed. ProductId: " << Utility::ConvertToUTF8(productId) << " HResult: " << errorCodeString);
            AICLI_TERMINATE_CONTEXT(errorCode);
        }
    }

    void MSStoreUpdate(Execution::Context& context)
    {
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);
        auto scope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
        HRESULT hr = MSStoreUpdateImpl(context, productId, scope, false);
        if (FAILED(hr))
        {
            AICLI_TERMINATE_CONTEXT(hr);
        }
    }

    void EnsureStorePolicySatisfied(Execution::Context& context)
    {
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);
        HRESULT hr = EnsureStorePolicySatisfiedImpl(context, productId);
        if (FAILED(hr))
        {
            AICLI_TERMINATE_CONTEXT(hr);
        }
    }

    void EnableConfiguration(Execution::Context& context)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        HRESULT hr = AppInstallerUpdate(context, false);
        if (FAILED(hr))
        {
            AICLI_TERMINATE_CONTEXT(hr);
        }
#else
        if (IsStubPackage())
        {
            context.Reporter.Info() << Resource::String::ConfigurationEnablingMessage << std::endl;
            HRESULT hr = AppInstallerUpdate(context, false);
            if (FAILED(hr))
            {
                AICLI_TERMINATE_CONTEXT(hr);
            }
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
        HRESULT hr = AppInstallerUpdate(context, true);
        if (FAILED(hr))
        {
            AICLI_TERMINATE_CONTEXT(hr);
        }
#else
        if (!IsStubPackage())
        {
            context.Reporter.Info() << Resource::String::ConfigurationDisablingMessage << std::endl;
            HRESULT hr = AppInstallerUpdate(context, true);
            if (FAILED(hr))
            {
                AICLI_TERMINATE_CONTEXT(hr);
            }
        }
        else
        {
            context.Reporter.Info() << Resource::String::ConfigurationDisabledMessage << std::endl;
        }
#endif
    }
}
