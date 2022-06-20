// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <pch.h>
#include <AppInstallerMsixInfo.h>
#include <winget/MsixManifest.h>

using namespace Microsoft::WRL;

namespace AppInstaller::Msix
{
    Utility::NormalizedString MsixPackageManifestIdentity::GetPackageFamilyName() const
    {
        wil::unique_cotaskmem_string familyName;
        THROW_IF_FAILED(m_packageId->GetPackageFamilyName(&familyName));
        return { familyName.get() };
    }

    PackageVersion MsixPackageManifestIdentity::GetVersion() const
    {
        UINT64 version = 0;
        THROW_IF_FAILED(m_packageId->GetVersion(&version));
        return PackageVersion{ version };
    }

    MsixPackageManifestIdentity MsixPackageManifest::GetIdentity() const
    {
        ComPtr<IAppxManifestPackageId> manifestPackageId;
        THROW_IF_FAILED(m_manifestReader->GetPackageId(&manifestPackageId));
        return MsixPackageManifestIdentity{ std::move(manifestPackageId) };
    }

    std::optional<OSVersion> MsixPackageManifest::GetMinimumOSVersionForSupportedPlatforms() const
    {
        std::optional<OSVersion> minOSVersion;
        auto targetDeviceFamilies = GetTargetDeviceFamilies();
        std::map<MsixPackageManifestTargetDeviceFamily::Platform, OSVersion> targetOSVersion;

        for (const auto& targetDeviceFamily : targetDeviceFamilies)
        {
            targetOSVersion.emplace(targetDeviceFamily.GetPlatform(), targetDeviceFamily.GetMinVersion());
        }

        if (targetOSVersion.find(MsixPackageManifestTargetDeviceFamily::Platform::WindowsDesktop) != targetOSVersion.end())
        {
            return targetOSVersion[MsixPackageManifestTargetDeviceFamily::Platform::WindowsDesktop];
        }

        if (targetOSVersion.find(MsixPackageManifestTargetDeviceFamily::Platform::WindowsUniversal) != targetOSVersion.end())
        {
            return targetOSVersion[MsixPackageManifestTargetDeviceFamily::Platform::WindowsUniversal];
        }

        return std::nullopt;
    }

    std::vector<MsixPackageManifestTargetDeviceFamily> MsixPackageManifest::GetTargetDeviceFamilies() const
    {
        std::vector<MsixPackageManifestTargetDeviceFamily> targetDeviceFamilies;
        ComPtr<IAppxManifestReader3> manifestReader3;
        THROW_IF_FAILED(m_manifestReader.As(&manifestReader3));

        ComPtr<IAppxManifestTargetDeviceFamiliesEnumerator> targetDeviceFamiliesIter;
        THROW_IF_FAILED(manifestReader3->GetTargetDeviceFamilies(&targetDeviceFamiliesIter));

        BOOL hasCurrent = FALSE;
        THROW_IF_FAILED(targetDeviceFamiliesIter->GetHasCurrent(&hasCurrent));
        while (hasCurrent)
        {
            ComPtr<IAppxManifestTargetDeviceFamily> targetDeviceFamily;
            THROW_IF_FAILED(targetDeviceFamiliesIter->GetCurrent(&targetDeviceFamily));

            targetDeviceFamilies.emplace_back(std::move(targetDeviceFamily));

            THROW_IF_FAILED(targetDeviceFamiliesIter->MoveNext(&hasCurrent));
        }

        return targetDeviceFamilies;
    }

    std::string MsixPackageManifestTargetDeviceFamily::GetName() const
    {
        wil::unique_cotaskmem_string name;
        THROW_IF_FAILED(m_targetDeviceFamily->GetName(&name));
        return Utility::ConvertToUTF8(name.get());
    }

    OSVersion MsixPackageManifestTargetDeviceFamily::GetMinVersion() const
    {
        UINT64 minVersion = 0;
        THROW_IF_FAILED(m_targetDeviceFamily->GetMinVersion(&minVersion));
        return OSVersion{ minVersion };
    }

    MsixPackageManifestTargetDeviceFamily::Platform MsixPackageManifestTargetDeviceFamily::GetPlatform() const
    {
        auto name = GetName();
        if (Utility::CaseInsensitiveEquals(name, WindowsDesktopName))
        {
            return WindowsDesktop;
        }

        if (Utility::CaseInsensitiveEquals(name, WindowsUniversalName))
        {
            return WindowsUniversal;
        }

        return Other;
    }

    const std::vector<MsixPackageManifest>& MsixPackageManifestCache::GetAppPackageManifests(std::string url)
    {
        // If an installer url has already been processed, then use the cached result
        auto installerIter = m_msixManifests.find(url);
        if (installerIter != m_msixManifests.end())
        {
            return installerIter->second;
        }

        MsixInfo msixInfo(url);

        // Cache installer url result
        m_msixManifests[url] = msixInfo.GetAppPackageManifests();
        return m_msixManifests[url];
    }
}
