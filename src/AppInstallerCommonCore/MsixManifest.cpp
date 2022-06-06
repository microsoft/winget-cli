// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <pch.h>
#include <AppInstallerMsixInfo.h>
#include <winget/MsixManifest.h>

using namespace Microsoft::WRL;

namespace AppInstaller::Msix
{
    MsixPackageManifest::MsixPackageManifest(ComPtr<IAppxManifestReader> manifestReader)
    {
        THROW_HR_IF(E_INVALIDARG, !manifestReader);
        m_manifestReader = manifestReader;
    }

    MsixPackageManifestIdentity::MsixPackageManifestIdentity(ComPtr<IAppxManifestPackageId> packageId)
    {
        THROW_HR_IF(E_INVALIDARG, !packageId);
        m_packageId = packageId;
    }

    MsixPackageManifestTargetDeviceFamily::MsixPackageManifestTargetDeviceFamily(ComPtr<IAppxManifestTargetDeviceFamily> targetDeviceFamily)
    {
        THROW_HR_IF(E_INVALIDARG, !targetDeviceFamily);
        m_targetDeviceFamily = targetDeviceFamily;
    }

    string_t MsixPackageManifestIdentity::GetPackageFamilyName() const
    {
        wil::unique_cotaskmem_string familyName;
        THROW_IF_FAILED(m_packageId->GetPackageFamilyName(&familyName));
        return Utility::ConvertToUTF8(familyName.get());
    }

    PackageVersion MsixPackageManifestIdentity::GetVersion() const
    {
        UINT64 version;
        THROW_IF_FAILED(m_packageId->GetVersion(&version));
        return PackageVersion{ version };
    }

    MsixPackageManifestIdentity MsixPackageManifest::GetIdentity() const
    {
        ComPtr<IAppxManifestPackageId> manifestPackageId;
        THROW_IF_FAILED(m_manifestReader->GetPackageId(&manifestPackageId));
        return MsixPackageManifestIdentity{ manifestPackageId };
    }

    std::optional<OSVersion> MsixPackageManifest::GetMinimumOSVersion() const
    {
        std::optional<OSVersion> minOSVersion;
        auto targetDeviceFamilies = GetTargetDeviceFamilies();
        for (const auto& targetDeviceFamily : targetDeviceFamilies)
        {
            auto targetDeviceFamilyMinOSVersion = targetDeviceFamily.GetMinVersion();
            if (!minOSVersion.has_value())
            {
                minOSVersion = targetDeviceFamilyMinOSVersion;
            }
            else if(targetDeviceFamilyMinOSVersion < minOSVersion.value())
            {
                minOSVersion = targetDeviceFamilyMinOSVersion;
            }
        }

        return minOSVersion;
    }

    std::vector<MsixPackageManifestTargetDeviceFamily> MsixPackageManifest::GetTargetDeviceFamilies() const
    {
        std::vector<MsixPackageManifestTargetDeviceFamily> targetDeviceFamilies;
        ComPtr<IAppxManifestReader3> manifest3;
        THROW_IF_FAILED(m_manifestReader->QueryInterface(IID_PPV_ARGS(&manifest3)));

        ComPtr<IAppxManifestTargetDeviceFamiliesEnumerator> targetDeviceFamiliesIter;
        THROW_IF_FAILED(manifest3->GetTargetDeviceFamilies(&targetDeviceFamiliesIter));

        BOOL hasCurrent = FALSE;
        THROW_IF_FAILED(targetDeviceFamiliesIter->GetHasCurrent(&hasCurrent));
        while (hasCurrent)
        {
            ComPtr<IAppxManifestTargetDeviceFamily> targetDeviceFamily;
            THROW_IF_FAILED(targetDeviceFamiliesIter->GetCurrent(&targetDeviceFamily));

            targetDeviceFamilies.emplace_back(targetDeviceFamily);

            THROW_IF_FAILED(targetDeviceFamiliesIter->MoveNext(&hasCurrent));
        }

        return targetDeviceFamilies;
    }

    string_t MsixPackageManifestTargetDeviceFamily::GetName() const
    {
        wil::unique_cotaskmem_string name;
        THROW_IF_FAILED(m_targetDeviceFamily->GetName(&name));
        return Utility::ConvertToUTF8(name.get());
    }

    OSVersion MsixPackageManifestTargetDeviceFamily::GetMinVersion() const
    {
        UINT64 minVersion;
        THROW_IF_FAILED(m_targetDeviceFamily->GetMinVersion(&minVersion));
        return OSVersion{ minVersion };
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
