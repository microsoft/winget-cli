// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <pch.h>
#include <AppInstallerMsixManifest.h>

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
        THROW_HR_IF(E_INVALIDARG, !manifestReader);
        m_manifestReader = manifestReader;

        THROW_IF_FAILED(m_manifestReader->GetPackageId(&m_manifestPackageId));

        Initialize();
    }

    void MsixPackageManifest::Initialize()
    {
        // Set package family name
        wil::unique_cotaskmem_string familyName;
        THROW_IF_FAILED(m_manifestPackageId->GetPackageFamilyName(&familyName));
		Identity.PackageFamilyName = Utility::ConvertToUTF8(familyName.get());

        // Set package version
        UINT64 version;
        THROW_IF_FAILED(m_manifestPackageId->GetVersion(&version));
		Identity.Version = version;

        // Set package dependencies
        ComPtr<IAppxManifestReader3> manifest3;
        if (SUCCEEDED(m_manifestReader->QueryInterface(IID_PPV_ARGS(&manifest3))))
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

                auto nameStr = Utility::ConvertToUTF8(name);
                auto manifestTargetDeviceFamily = MsixPackageManifestTargetDeviceFamily(nameStr, minVersion, maxVersionTested);
                if (Platform::IsWindowsDesktop(nameStr))
                {
                    Dependencies.WindowsDesktop = manifestTargetDeviceFamily;
                }
                else if (Platform::IsWindowsUniversal(nameStr))
                {
                    Dependencies.WindowsUniversal = manifestTargetDeviceFamily;
                }
            }
        }
    }
}

#undef WINDOWS_DESKTOP
