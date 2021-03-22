#pragma once
#include "InstallOptions.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct InstallOptions : InstallOptionsT<InstallOptions>
    {
        InstallOptions() = default;

        Microsoft::Management::Deployment::PackageUniqueId Id();
        void Id(Microsoft::Management::Deployment::PackageUniqueId const& value);
        hstring Manifest();
        void Manifest(hstring const& value);
        hstring InstallLocation();
        void InstallLocation(hstring const& value);
        hstring SessionId();
        void SessionId(hstring const& value);
        Microsoft::Management::Deployment::InstallScope InstallScope();
        void InstallScope(Microsoft::Management::Deployment::InstallScope const& value);

    private:
        Microsoft::Management::Deployment::PackageUniqueId m_uniqueId;
        std::wstring m_manifest = L"";
        std::wstring m_installLocation = L"";
        std::wstring m_sessionId = L"";
        Microsoft::Management::Deployment::InstallScope m_installScope = Microsoft::Management::Deployment::InstallScope::User;
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct InstallOptions : InstallOptionsT<InstallOptions, implementation::InstallOptions>
    {
    };
}
