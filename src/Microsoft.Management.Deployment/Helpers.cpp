// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <winrt/Windows.Security.Authorization.AppCapabilityAccess.h>
#include <appmodel.h>
#include <Helpers.h>
#include <winget/Security.h>

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace winrt::Microsoft::Management::Deployment::implementation
{
    namespace
    {
        static std::optional<std::string> s_callerName;
        static wil::srwlock s_callerNameLock;
    }

    void SetComCallerName(std::string name)
    {
        auto lock = s_callerNameLock.lock_exclusive();
        s_callerName.emplace(std::move(name));
    }

    std::string GetComCallerName(std::string defaultNameIfNotSet)
    {
        auto lock = s_callerNameLock.lock_shared();
        return s_callerName.has_value() ? s_callerName.value() : defaultNameIfNotSet;
    }

    std::pair<HRESULT, DWORD> GetCallerProcessId()
    {
        RPC_STATUS rpcStatus = RPC_S_OK;
        RPC_CALL_ATTRIBUTES callAttributes = {};
        callAttributes.Version = RPC_CALL_ATTRIBUTES_VERSION;
        callAttributes.Flags = RPC_QUERY_CLIENT_PID;
        rpcStatus = RpcServerInqCallAttributes(nullptr, &callAttributes);

        if (rpcStatus == RPC_S_NO_CALL_ACTIVE ||
            (rpcStatus == RPC_S_OK && HandleToULong(callAttributes.ClientPID) == GetCurrentProcessId()))
        {
            // in-proc is supported now.
            return { S_OK, GetCurrentProcessId() };
        }
        else if (rpcStatus == RPC_S_OK)
        {
            // out-of-proc case.
            return { S_OK, HandleToULong(callAttributes.ClientPID) };
        }
        else
        {
            return { E_ACCESSDENIED, 0 };
        }
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
        bool allowed = false;

        if (winrt::Windows::Foundation::Metadata::ApiInformation::IsTypePresent(winrt::name_of<winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapability>()))
        {
            // Get the caller process id and use it to check if the caller has permissions to access the feature.
            winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapabilityAccessStatus status = winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapabilityAccessStatus::DeniedBySystem;

            auto capability = winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapability::CreateWithProcessIdForUser(nullptr, GetStringForCapability(requiredCapability), callerProcessId);
            status = capability.CheckAccess();

            allowed = (status == winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapabilityAccessStatus::Allowed);
        }
        else
        {
            // If AppCapability is not present, require at least medium IL callers
            auto requiredIntegrityLevel = AppInstaller::Security::IntegrityLevel::Medium;

            if (callerProcessId != GetCurrentProcessId())
            {
                allowed = AppInstaller::Security::IsCOMCallerIntegrityLevelAtLeast(requiredIntegrityLevel);
            }
            else
            {
                allowed = AppInstaller::Security::IsCurrentIntegrityLevelAtLeast(requiredIntegrityLevel);
            }
        }

        return (allowed ? S_OK : E_ACCESSDENIED);
    }

    HRESULT EnsureComCallerHasCapability(Capability requiredCapability)
    {
        auto [hr, callerProcessId] = GetCallerProcessId();
        RETURN_IF_FAILED(hr);
        hr = EnsureProcessHasCapability(requiredCapability, callerProcessId);
        // The Windows.Management.Deployment API has set the precedent that packageManagement is a superset of packageQuery
        // and packageQuery does not need to be declared separately.
        if (FAILED(hr) && requiredCapability == Capability::PackageQuery)
        {
            hr = EnsureProcessHasCapability(Capability::PackageManagement, callerProcessId);
        }
        RETURN_HR(hr);
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

    std::string GetCallerName()
    {
        // See if caller name is set by caller
        std::string callerName = GetComCallerName("");

        // Get process string
        if (callerName.empty())
        {
            try
            {
                auto [hrGetCallerId, callerProcessId] = GetCallerProcessId();
                if (SUCCEEDED(hrGetCallerId))
                {
                    callerName = AppInstaller::Utility::ConvertToUTF8(TryGetCallerProcessInfo(callerProcessId));
                }
            }
            CATCH_LOG();
        }

        if (callerName.empty())
        {
            callerName = "UnknownComCaller";
        }

        return callerName;
    }

    bool IsBackgroundProcessForPolicy()
    {
        bool isBackgroundProcessForPolicy = false;
        try
        {
            auto [hrGetCallerId, callerProcessId] = GetCallerProcessId();
            if (SUCCEEDED(hrGetCallerId) && callerProcessId != GetCurrentProcessId())
            {
                // OutOfProc case, we check for explorer.exe
                auto callerNameWide = AppInstaller::Utility::ConvertToUTF16(GetCallerName());
                auto processName = AppInstaller::Utility::ConvertToUTF8(std::filesystem::path{ callerNameWide }.filename().wstring());
                if (::AppInstaller::Utility::CaseInsensitiveEquals("explorer.exe", processName) ||
                    ::AppInstaller::Utility::CaseInsensitiveEquals("taskhostw.exe", processName))
                {
                    isBackgroundProcessForPolicy = true;
                }
            }
        }
        CATCH_LOG();

        return isBackgroundProcessForPolicy;
    }
}
