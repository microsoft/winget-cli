// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <pch.h>
#include <AppInstallerMsixManifest.h>
#include <AppInstallerMsixInfo.h>

namespace AppInstaller::Msix
{
    bool Platform::IsWindowsDesktop(std::string platformName)
    {
        return Utility::CaseInsensitiveEquals(platformName, Platform::WindowsDesktop);
    }

    bool Platform::IsWindowsUniversal(std::string platformName)
    {
        return Utility::CaseInsensitiveEquals(platformName, Platform::WindowsUniversal);
    }

    MsixPackageManifest::MsixPackageManifest(ComPtr<IAppxManifestReader> manifestReader)
    {
        Assign(manifestReader);
    }

    void MsixPackageManifest::Assign(ComPtr<IAppxManifestReader> manifestReader)
    {
        THROW_HR_IF(E_INVALIDARG, !manifestReader);

        ComPtr<IAppxManifestPackageId> manifestPackageId;
        THROW_IF_FAILED(manifestReader->GetPackageId(&manifestPackageId));

        // Set package family name
        wil::unique_cotaskmem_string familyName;
        THROW_IF_FAILED(manifestPackageId->GetPackageFamilyName(&familyName));
        Identity.PackageFamilyName = Utility::ConvertToUTF8(familyName.get());

        // Set package version
        UINT64 version;
        THROW_IF_FAILED(manifestPackageId->GetVersion(&version));
        Identity.Version = version;

        // Set package dependencies
        ComPtr<IAppxManifestReader3> manifest3;
        if (SUCCEEDED(manifestReader->QueryInterface(IID_PPV_ARGS(&manifest3))))
        {
            ComPtr<IAppxManifestTargetDeviceFamiliesEnumerator> targetDeviceFamilies;
            THROW_IF_FAILED(manifest3->GetTargetDeviceFamilies(&targetDeviceFamilies));

            BOOL hasCurrent = FALSE;
            THROW_IF_FAILED(targetDeviceFamilies->GetHasCurrent(&hasCurrent));
            while (hasCurrent)
            {
                ComPtr<IAppxManifestTargetDeviceFamily> targetDeviceFamily;
                THROW_IF_FAILED(targetDeviceFamilies->GetCurrent(&targetDeviceFamily));
                
                LPWSTR name;
                THROW_IF_FAILED(targetDeviceFamily->GetName(&name));

                UINT64 minVersion;
                THROW_IF_FAILED(targetDeviceFamily->GetMinVersion(&minVersion));

                UINT64 maxVersionTested;
                THROW_IF_FAILED(targetDeviceFamily->GetMaxVersionTested(&maxVersionTested));

                Dependencies.TargetDeviceFamilies.emplace_back(Utility::ConvertToUTF8(name), minVersion, maxVersionTested);

                THROW_IF_FAILED(targetDeviceFamilies->MoveNext(&hasCurrent));
            }
        }
    }

    const std::vector<MsixPackageManifest>& MsixPackageManifestManager::GetAppPackageManifests(std::string url)
    {
        // If an installer url has already been processed, then use the memoized result
        auto installerIter = m_msixManifests.find(url);
        if (installerIter != m_msixManifests.end())
        {
            return installerIter->second;
        }

        MsixInfo msixInfo(url);

        // Memoize installer url result
        m_msixManifests[url] = msixInfo.GetAppPackageManifests();
        return m_msixManifests[url];
    }
}
