#include "pch.h"
#include "InstallOptions.h"
#include "InstallOptions.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Microsoft::Management::Deployment::PackageUniqueId InstallOptions::Id()
    {
        return m_uniqueId;
    }
    void InstallOptions::Id(Microsoft::Management::Deployment::PackageUniqueId const& value)
    {
        m_uniqueId = PackageUniqueId(value);
    }
    hstring InstallOptions::Manifest()
    {
        return hstring(m_manifest);
    }
    void InstallOptions::Manifest(hstring const& value)
    {
        m_manifest.assign(value.c_str());
    }
    hstring InstallOptions::InstallLocation()
    {
        return hstring(m_installLocation);
    }
    void InstallOptions::InstallLocation(hstring const& value)
    {
        m_installLocation.assign(value.c_str());
    }
    hstring InstallOptions::SessionId()
    {
        return hstring(m_sessionId);
    }
    void InstallOptions::SessionId(hstring const& value)
    {
        m_sessionId.assign(value.c_str());
    }
    Microsoft::Management::Deployment::InstallScope InstallOptions::InstallScope()
    {
        return m_installScope;
    }
    void InstallOptions::InstallScope(Microsoft::Management::Deployment::InstallScope const& value)
    {
        m_installScope = value;
    }
}
