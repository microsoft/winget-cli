
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <winrt/Windows.Security.Authorization.AppCapabilityAccess.h>
#include <appmodel.h>
#include <Helpers.h>

using namespace std::string_literals;
using namespace std::string_view_literals;

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

    std::wstring_view GetStringForCapability(Capability capability)
    {
        switch (capability)
        {
        case Capability::PackageManagement:
            return L"packageManagement"sv;
        case Capability::PackageQuery:
            return L"packageQuery"sv;
        default:
            winrt::throw_hresult(E_UNEXPECTED);
        }
    }

    HRESULT EnsureProcessHasCapability(Capability requiredCapability, DWORD callerProcessId)
    {
        // Get the caller process id and use it to check if the caller has permissions to access the feature.
        winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapabilityAccessStatus status = winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapabilityAccessStatus::DeniedBySystem;

        auto capability = winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapability::CreateWithProcessIdForUser(nullptr, GetStringForCapability(requiredCapability), callerProcessId);
        status = capability.CheckAccess();
        RETURN_HR_IF(E_ACCESSDENIED, status != winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapabilityAccessStatus::Allowed);

        return S_OK;
    }

    HRESULT EnsureComCallerHasCapability(Capability requiredCapability)
    {
        auto callerProcessId = GetCallerProcessId();
        RETURN_HR_IF(E_ACCESSDENIED, !callerProcessId.has_value());
        HRESULT hr = EnsureProcessHasCapability(requiredCapability, callerProcessId.value());
        // The Windows.Management.Deployment API has set the precedent that packageManagement is a superset of packageQuery
        // and packageQuery does not need to be declared separately.
        if (FAILED(hr) && requiredCapability == Capability::PackageQuery)
        {
            return EnsureProcessHasCapability(Capability::PackageManagement, callerProcessId.value());
        }
        return hr;
    }

    // Best effort at getting caller info. This should only be used for logging.
    std::wstring TryGetCallerProcessInfo(DWORD callerProcessId)
    {
        wil::unique_process_handle processHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, callerProcessId));
        if (processHandle)
        {
            WCHAR packageFamilyName[PACKAGE_FAMILY_NAME_MAX_LENGTH]{};
            UINT32 length = ARRAYSIZE(packageFamilyName);
            if (::GetPackageFamilyName(processHandle.get(), &length, packageFamilyName) == ERROR_SUCCESS)
            {
                return { packageFamilyName };
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