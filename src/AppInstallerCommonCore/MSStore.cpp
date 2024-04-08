// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <winget/MSStore.h>
#include <winget/ManifestCommon.h>
#include <winget/Runtime.h>
#include <AppInstallerFileLogger.h>
#include <AppInstallerErrors.h>

namespace AppInstaller::MSStore
{
    using namespace std::string_view_literals;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::ApplicationModel::Store::Preview::InstallControl;

    namespace
    {
        // The type of entitlement we were able to acquire/ensure.
        enum class EntitlementType
        {
            None,
            User,
            Device,
        };

        EntitlementType EnsureFreeEntitlement(const std::wstring& productId, Manifest::ScopeEnum scope)
        {
            AppInstallManager installManager;

            AICLI_LOG(Core, Info, << "Getting entitlement for ProductId: " << Utility::ConvertToUTF8(productId));

            // Verifying/Acquiring product ownership
            GetEntitlementResult entitlementResult{ nullptr };
            EntitlementType result = EntitlementType::None;

            if (scope == Manifest::ScopeEnum::Machine)
            {
                AICLI_LOG(Core, Info, << "Get device entitlement (machine scope install).");
                result = EntitlementType::Device;
                try
                {
                    entitlementResult = installManager.GetFreeDeviceEntitlementAsync(productId, winrt::hstring(), winrt::hstring()).get();
                }
                CATCH_LOG();
            }
            else
            {
                AICLI_LOG(Core, Info, << "Get user entitlement.");
                result = EntitlementType::User;
                try
                {
                    entitlementResult = installManager.GetFreeUserEntitlementAsync(productId, winrt::hstring(), winrt::hstring()).get();
                }
                CATCH_LOG();

                if (!entitlementResult || entitlementResult.Status() == GetEntitlementStatus::NoStoreAccount)
                {
                    AICLI_LOG(Core, Info, << "Get device entitlement (no store account).");
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
                AICLI_LOG(Core, Info, << "Get entitlement succeeded.");
            }
            else if (entitlementResult)
            {
                result = EntitlementType::None;

                if (entitlementResult.Status() == GetEntitlementStatus::NetworkError)
                {
                    AICLI_LOG(Core, Error, << "Get entitlement failed. Network error.");
                }
                else if (entitlementResult.Status() == GetEntitlementStatus::ServerError)
                {
                    AICLI_LOG(Core, Error, << "Get entitlement failed. Server error.");
                }
                else
                {
                    AICLI_LOG(Core, Error, << "Get entitlement failed. Unknown status: " << static_cast<int32_t>(entitlementResult.Status()));
                }
            }
            else
            {
                result = EntitlementType::None;
                AICLI_LOG(Core, Error, << "Get entitlement failed. Exception.");
            }

            return result;
        }

        enum class CheckExistingItemResult
        {
            None,
            Restart,
            Cancel,
        };

        CheckExistingItemResult CheckRestartOrCancelForPossibleExistingOperation(const IVectorView<AppInstallItem>& installItems)
        {
            CheckExistingItemResult result = CheckExistingItemResult::None;

            for (auto const& installItem : installItems)
            {
                const auto& status = installItem.GetCurrentStatus();
                switch (status.InstallState())
                {
                case AppInstallState::Canceled:
                case AppInstallState::Error:
                    // For these states, always do a cancel;
                    result = CheckExistingItemResult::Cancel;
                    return result;
                case AppInstallState::Paused:
                case AppInstallState::PausedLowBattery:
                case AppInstallState::PausedWiFiRecommended:
                case AppInstallState::PausedWiFiRequired:
                case AppInstallState::ReadyToDownload:
                    // For these states, set result to restart and continue the loop to see if future items need cancel.
                    result = CheckExistingItemResult::Restart;
                    break;
                }
            }

            return result;
        }

        bool DoesInstallItemsContainProduct(const IVectorView<AppInstallItem>& installItems, std::wstring_view productId)
        {
            for (auto const& installItem : installItems)
            {
                if (Utility::CaseInsensitiveEquals(installItem.ProductId(), productId))
                {
                    return true;
                }
            }

            return false;
        }

        // Returns true if Restart or Cancel happened. False otherwise.
        HRESULT RestartOrCancelExistingOperationIfNecessary(const IVectorView<AppInstallItem>& installItems, AppInstallManager& installManager, std::wstring_view productId)
        {
            auto existingItemResult = CheckRestartOrCancelForPossibleExistingOperation(installItems);

            if (existingItemResult == CheckExistingItemResult::Cancel || existingItemResult == CheckExistingItemResult::Restart)
            {
                if (existingItemResult == CheckExistingItemResult::Cancel)
                {
                    installManager.Cancel(productId);

                    // Wait for at most 10 seconds for install item to be removed from queue.
                    for (int i = 0; i < 50; ++i)
                    {
                        Sleep(200);
                        if (!DoesInstallItemsContainProduct(installManager.AppInstallItems(), productId))
                        {
                            return S_OK;
                        }
                    }

                    RETURN_HR(HRESULT_FROM_WIN32(ERROR_TIMEOUT));
                }
                else
                {
                    installManager.Restart(productId);
                    return S_OK;
                }
            }

            return S_FALSE;
        }
    }

    HRESULT MSStoreOperation::StartAndWaitForOperation(IProgressCallback& progress)
    {
        // Best effort verifying/acquiring product ownership.
        std::ignore = EnsureFreeEntitlement(m_productId, m_scope);

        if (m_type == MSStoreOperationType::Update)
        {
            return UpdatePackage(progress);
        }
        else
        {
            return InstallPackage(progress);
        }
    }

    HRESULT MSStoreOperation::InstallPackage(IProgressCallback& progress)
    {
        AppInstallManager installManager;
        AppInstallOptions installOptions;

        installOptions.AllowForcedAppRestart(m_force);
        if (m_isSilentMode)
        {
            installOptions.InstallInProgressToastNotificationMode(AppInstallationToastNotificationMode::NoToast);
            installOptions.CompletedInstallToastNotificationMode(AppInstallationToastNotificationMode::NoToast);
        }

        if (m_type == MSStoreOperationType::Repair)
        {
            // Attempt to repair the installation of an app that is already installed.
            installOptions.Repair(true);
        }

        if (m_scope == Manifest::ScopeEnum::Machine)
        {
            // TODO: There was a bug in InstallService where admin user is incorrectly identified as not admin,
            // causing false access denied on many OS versions.
            // Remove this check when the OS bug is fixed and back ported.
            if (!Runtime::IsRunningAsSystem())
            {
                AICLI_LOG(Core, Error, << "Device wide install for msstore type is not supported under admin context.");
                return APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED;
            }

            installOptions.InstallForAllUsers(true);
        }

        IVectorView<AppInstallItem> installItems = installManager.StartProductInstallAsync(
            m_productId,            // ProductId
            winrt::hstring(),       // FlightId
            L"WinGetCli",           // ClientId
            winrt::hstring(),
            installOptions).get();

        // Check if we need to restart or cancel existing items.
        auto restartOrCancelResult = RestartOrCancelExistingOperationIfNecessary(installItems, installManager, m_productId);
        RETURN_IF_FAILED(restartOrCancelResult);

        // If restart or cancel happened, try again.
        if (restartOrCancelResult == S_OK)
        {
            // Try again
            installItems = installManager.StartProductInstallAsync(
                m_productId,            // ProductId
                winrt::hstring(),       // FlightId
                L"WinGetCli",           // ClientId
                winrt::hstring(),
                installOptions).get();
        }

        return WaitForOperation(installItems, progress);
    }

    HRESULT MSStoreOperation::UpdatePackage(IProgressCallback& progress)
    {
        AppInstallManager installManager;
        AppUpdateOptions updateOptions;
        updateOptions.AllowForcedAppRestart(m_force);

        // SearchForUpdateAsync will automatically trigger update if found.
        AppInstallItem installItem = installManager.SearchForUpdatesAsync(
            m_productId,          // ProductId
            winrt::hstring(),   // SkuId
            winrt::hstring(),
            winrt::hstring(),   // ClientId
            updateOptions
        ).get();

        if (!installItem)
        {
            return APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE;
        }

        std::vector<AppInstallItem> installItemVector{ installItem };
        IVectorView<AppInstallItem> installItems = winrt::single_threaded_vector(std::move(installItemVector)).GetView();

        // Check if we need to restart or cancel existing items.
        auto restartOrCancelResult = RestartOrCancelExistingOperationIfNecessary(installItems, installManager, m_productId);
        RETURN_IF_FAILED(restartOrCancelResult);

        // If restart or cancel happened, try again.
        if (restartOrCancelResult == S_OK)
        {
            // Try again
            installItem = installManager.SearchForUpdatesAsync(
                m_productId,          // ProductId
                winrt::hstring(),   // SkuId
                winrt::hstring(),
                winrt::hstring(),   // ClientId
                updateOptions
            ).get();

            if (!installItem)
            {
                return APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE;
            }

            installItemVector.clear();
            installItemVector.emplace_back(installItem);
            installItems = winrt::single_threaded_vector(std::move(installItemVector)).GetView();
        }

        return WaitForOperation(installItems, progress);
    }

    HRESULT MSStoreOperation::WaitForOperation(IVectorView<AppInstallItem>& installItems, IProgressCallback& progress)
    {
        auto cancelIfOperationFailed = wil::scope_exit(
            [&]()
            {
                try
                {
                    AppInstallManager installManager;
                    installManager.Cancel(m_productId);
                }
                CATCH_LOG();
            });

        for (auto const& installItem : installItems)
        {
            AICLI_LOG(Core, Info, <<
                "Started MSStore package execution. ProductId: " << Utility::ConvertToUTF8(installItem.ProductId()) <<
                " PackageFamilyName: " << Utility::ConvertToUTF8(installItem.PackageFamilyName()));

            if (m_isSilentMode)
            {
                installItem.InstallInProgressToastNotificationMode(AppInstallationToastNotificationMode::NoToast);
                installItem.CompletedInstallToastNotificationMode(AppInstallationToastNotificationMode::NoToast);
            }
        }

        HRESULT errorCode = S_OK;

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
                    return errorCode;
                }
            }

            // It may take a while for Store client to pick up the install request.
            // So we show indefinite progress here to avoid a progress bar stuck at 0.
            if (currentProgress > 0)
            {
                progress.OnProgress(currentProgress, overallProgressMax, ProgressType::Percent);
            }

            if (progress.IsCancelledBy(CancelReason::User))
            {
                for (auto const& installItem : installItems)
                {
                    installItem.Cancel();
                }
            }

            // If app shutdown then we have 30s to keep installing, keep going and hope for the best.
            else if (progress.IsCancelledBy(CancelReason::AppShutdown))
            {
                for (auto const& installItem : installItems)
                {
                    // Insert spiderman meme.
                    if (installItem.ProductId() == std::wstring{ s_AppInstallerProductId })
                    {
                        AICLI_LOG(Core, Info, << "Asked to shutdown while installing AppInstaller.");
                        progress.OnProgress(overallProgressMax, overallProgressMax, ProgressType::Percent);
                        cancelIfOperationFailed.release();
                        return S_OK;
                    }
                }
            }

            Sleep(100);
        }

        if (SUCCEEDED(errorCode))
        {
            cancelIfOperationFailed.release();
        }

        return errorCode;
    }
}
