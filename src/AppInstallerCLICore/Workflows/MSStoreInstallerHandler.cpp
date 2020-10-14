// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
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
            for (auto const& installItem : installItems)
            {
                AICLI_LOG(CLI, Info, <<
                    "Started MSStore package execution. ProductId: " << Utility::ConvertToUTF8(installItem.ProductId()) <<
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

            return errorCode;
        }
    }

    void MSStoreInstall(Execution::Context& context)
    {
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);

        AppInstallManager installManager;

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

        HRESULT errorCode = WaitForMSStoreOperation(context, installItems);

        if (SUCCEEDED(errorCode))
        {
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
        else
        {
            context.Reporter.Info() << Resource::String::MSStoreInstallOrUpdateFailed << ' ' << WINGET_OSTREAM_FORMAT_HRESULT(errorCode) << std::endl;
            AICLI_LOG(CLI, Error, << "MSStore install failed. ProductId: " << Utility::ConvertToUTF8(productId) << " HResult: " << WINGET_OSTREAM_FORMAT_HRESULT(errorCode));
            AICLI_TERMINATE_CONTEXT(errorCode);
        }
    }

    void MSStoreUpdate(Execution::Context& context)
    {
        auto productId = Utility::ConvertToUTF16(context.Get<Execution::Data::Installer>()->ProductId);

        AppInstallManager installManager;

        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        // SearchForUpdateAsync will automatically trigger update if found.
        AppInstallItem installItem = installManager.SearchForUpdatesAsync(
            productId,          // ProductId
            winrt::hstring()    // SkuId
            ).get();

        if (!installItem)
        {
            context.Reporter.Info() << Resource::String::UpdateNotApplicable << std::endl;
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
            context.Reporter.Info() << Resource::String::MSStoreInstallOrUpdateFailed << ' ' << WINGET_OSTREAM_FORMAT_HRESULT(errorCode) << std::endl;
            AICLI_LOG(CLI, Error, << "MSStore execution failed. ProductId: " << Utility::ConvertToUTF8(productId) << " HResult: " << WINGET_OSTREAM_FORMAT_HRESULT(errorCode));
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
        if (installManager.IsStoreBlockedByPolicyAsync(s_StoreClientName, s_StoreClientPublisher).get())
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