// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <winget/ManifestCommon.h>
#include "MSStoreInstallerHandler.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace std::string_view_literals;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::ApplicationModel::Store::Preview::InstallControl;

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

        bool GetFreeEntitlement(Execution::Context& context, const std::wstring& productId)
        {
            AppInstallManager installManager;

            // Verifying/Acquiring product ownership
            context.Reporter.Info() << Resource::String::MSStoreInstallTryGetEntitlement << std::endl;

            GetEntitlementResult result{ nullptr };

            if (Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope)) == Manifest::ScopeEnum::Machine)
            {
                AICLI_LOG(CLI, Info, << "Get device entitlement.");
                result = installManager.GetFreeDeviceEntitlementAsync(productId, winrt::hstring(), winrt::hstring()).get();
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Get user entitlement.");
                result = installManager.GetFreeUserEntitlementAsync(productId, winrt::hstring(), winrt::hstring()).get();
                if (result.Status() == GetEntitlementStatus::NoStoreAccount)
                {
                    AICLI_LOG(CLI, Info, << "Get device entitlement.");
                    result = installManager.GetFreeDeviceEntitlementAsync(productId, winrt::hstring(), winrt::hstring()).get();
                }
            }

            if (result.Status() == GetEntitlementStatus::Succeeded)
            {
                context.Reporter.Info() << Resource::String::MSStoreInstallGetEntitlementSuccess << std::endl;
                AICLI_LOG(CLI, Info, << "Get entitlement succeeded.");
            }
            else if (result.Status() == GetEntitlementStatus::NetworkError)
            {
                context.Reporter.Info() << Resource::String::MSStoreInstallGetEntitlementNetworkError << std::endl;
                AICLI_LOG(CLI, Error, << "Get entitlement failed. Network error.");
            }
            else if (result.Status() == GetEntitlementStatus::ServerError)
            {
                context.Reporter.Info() << Resource::String::MSStoreInstallGetEntitlementServerError << std::endl;
                AICLI_LOG(CLI, Error, << "Get entitlement failed Server error. ProductId: " << Utility::ConvertToUTF8(productId));
            }

            return result.Status() == GetEntitlementStatus::Succeeded;
        }
    }

    Utility::LocIndString GetErrorCodeString(const HRESULT errorCode)
    {
        std::ostringstream ssError;
        ssError << WINGET_OSTREAM_FORMAT_HRESULT(errorCode);
        return Utility::LocIndString{ ssError.str() };
    }

    void MSStoreInstall(Execution::Context& context)
    {
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);

        // Verifying/Acquiring product ownership
        if (!GetFreeEntitlement(context, productId))
        {
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_MSSTORE_INSTALL_FAILED);
        }

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
            context.Reporter.Info() << Resource::String::MSStoreInstallOrUpdateFailed(errorCodeString) << std::endl;
            context.Add<Execution::Data::OperationReturnCode>(errorCode);
            AICLI_LOG(CLI, Error, << "MSStore install failed. ProductId: " << Utility::ConvertToUTF8(productId) << " HResult: " << errorCodeString);
            AICLI_TERMINATE_CONTEXT(errorCode);
        }
    }

    void MSStoreUpdate(Execution::Context& context)
    {
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);

        // Verifying/Acquiring product ownership
        if (!GetFreeEntitlement(context, productId))
        {
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_MSSTORE_INSTALL_FAILED);
        }

        AppInstallManager installManager;
        AppUpdateOptions updateOptions;

        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        // SearchForUpdateAsync will automatically trigger update if found.
        AppInstallItem installItem = installManager.SearchForUpdatesAsync(
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
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
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
            AICLI_TERMINATE_CONTEXT(errorCode);
        }
    }

    void EnsureStorePolicySatisfied(Execution::Context& context)
    {
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);

        constexpr std::wstring_view s_StoreClientName = L"Microsoft.WindowsStore"sv;
        constexpr std::wstring_view s_StoreClientPublisher = L"CN=Microsoft Corporation, O=Microsoft Corporation, L=Redmond, S=Washington, C=US"sv;

        // Policy check
        AppInstallManager installManager; 
        
        if (!WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::BypassIsStoreClientBlockedPolicyCheck) && installManager.IsStoreBlockedByPolicyAsync(s_StoreClientName, s_StoreClientPublisher).get())
        {
            context.Reporter.Error() << Resource::String::MSStoreStoreClientBlocked << std::endl;
            AICLI_LOG(CLI, Error, << "Store client is blocked by policy. MSStore execution failed.");
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_MSSTORE_BLOCKED_BY_POLICY);
        }

        if (!installManager.GetIsAppAllowedToInstallAsync(productId).get())
        {
            context.Reporter.Error() << Resource::String::MSStoreAppBlocked << std::endl;
            AICLI_LOG(CLI, Error, << "App is blocked by policy. MSStore execution failed. ProductId: " << Utility::ConvertToUTF8(productId));
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_MSSTORE_APP_BLOCKED_BY_POLICY);
        }
    }
}
