// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/CoCreatableMicrosoftManagementDeploymentClass.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void SetComCallerName(std::string name);
    std::string GetComCallerName(std::string defaultNameIfNotSet);

    enum class Capability
    {
        PackageManagement,
        PackageQuery
    };

    HRESULT EnsureProcessHasCapability(Capability requiredCapability, DWORD callerProcessId);
    HRESULT EnsureComCallerHasCapability(Capability requiredCapability);
    std::pair<HRESULT, DWORD> GetCallerProcessId();
    std::wstring TryGetCallerProcessInfo(DWORD callerProcessId);
    std::string GetCallerName();
    bool IsBackgroundProcessForPolicy();
}
