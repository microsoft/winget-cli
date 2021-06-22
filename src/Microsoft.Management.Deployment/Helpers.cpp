
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <winrt/Windows.Security.Authorization.AppCapabilityAccess.h>
#include <appmodel.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    std::optional<DWORD> GetCallerProcessId()
    {
        RPC_STATUS rpcStatus = RPC_S_OK;
        RPC_CALL_ATTRIBUTES callAttributes = {};
        callAttributes.Version = RPC_CALL_ATTRIBUTES_VERSION;
        callAttributes.Flags = RPC_QUERY_CLIENT_PID;
        rpcStatus = RpcServerInqCallAttributes(nullptr, &callAttributes);

        if ((rpcStatus != RPC_S_NO_CALL_ACTIVE) &&
            !((rpcStatus == RPC_S_OK) && HandleToULong(callAttributes.ClientPID) == GetCurrentProcessId()))
        {
            DWORD callerProcessId = HandleToULong(callAttributes.ClientPID);
            return callerProcessId;
        }
        return {};
    }

    HRESULT EnsureProcessHasCapability(DWORD callerProcessId)
    {
        // Get the caller process id and use it to check if the caller has permissions to access the feature.
        winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapabilityAccessStatus status = winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapabilityAccessStatus::DeniedBySystem;

        auto capability = winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapability::CreateWithProcessIdForUser(nullptr, L"packageManagement", callerProcessId);
        status = capability.CheckAccess();
        RETURN_HR_IF(E_ACCESSDENIED, status != winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapabilityAccessStatus::Allowed);

        return S_OK;
    }

    HRESULT EnsureComCallerHasCapability()
    {
        auto callerProcessId = GetCallerProcessId();
        RETURN_HR_IF(E_ACCESSDENIED, !callerProcessId.has_value());
        return EnsureProcessHasCapability(callerProcessId.value());
    }

    std::wstring GetAppUserModelIdFromToken(HANDLE token)
    {
        WCHAR appUserModelID[APPLICATION_USER_MODEL_ID_MAX_LENGTH]{};
        UINT32 length = ARRAYSIZE(appUserModelID);
        if (::GetApplicationUserModelIdFromToken(token, &length, appUserModelID) == ERROR_SUCCESS)
        {
            return { appUserModelID };
        }
        return {};
    }

    // Best effort at getting caller info. This should only be used for logging.
    std::wstring TryGetCallerProcessInfo(DWORD callerProcessId)
    {
        wil::unique_process_handle processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, callerProcessId));
        if (processHandle)
        {
            HANDLE token;
            if (OpenProcessToken(processHandle.get(), TOKEN_QUERY, &token))
            {
                std::wstring appUserModelID = GetAppUserModelIdFromToken(token);
                if (!appUserModelID.empty())
                {
                    // The AppUserModelID is of the format <PackageFamilyName>!<ApplicationName>, only the package family name is of interest
                    auto index = appUserModelID.find('!');
                    if (index > 0)
                    {
                        return appUserModelID.substr(0, index);
                    }
                    return appUserModelID;
                }
            }

            // if the caller doesn't have an AppUserModelID then fall back to the executable name
            wil::unique_cotaskmem_string imageName = nullptr;
            if (SUCCEEDED(wil::QueryFullProcessImageNameW(processHandle.get(), 0, imageName)) &&
                (imageName.get() != nullptr))
            {
                return imageName.get();
            }
        }

        return {};
    }
}