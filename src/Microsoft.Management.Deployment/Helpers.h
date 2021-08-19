#pragma once

namespace winrt::Microsoft::Management::Deployment::implementation
{
    enum class Capability
    {
        PackageManagement,
        PackageQuery
    };

    HRESULT EnsureProcessHasCapability(Capability requiredCapability, DWORD callerProcessId);
    HRESULT EnsureComCallerHasCapability(Capability requiredCapability);
    std::optional<DWORD> GetCallerProcessId();
    std::wstring TryGetCallerProcessInfo(DWORD callerProcessId);
}