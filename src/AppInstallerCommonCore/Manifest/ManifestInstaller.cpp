// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/ManifestInstaller.h"

namespace AppInstaller::Manifest
{
    ManifestInstaller::InstallerTypeEnum ManifestInstaller::ConvertToInstallerTypeEnum(const std::string& in)
    {
        std::string inStrLower = Utility::ToLower(in);
        InstallerTypeEnum result = InstallerTypeEnum::Unknown;

        if (inStrLower == "inno")
        {
            result = InstallerTypeEnum::Inno;
        }
        else if (inStrLower == "wix")
        {
            result = InstallerTypeEnum::Wix;
        }
        else if (inStrLower == "msi")
        {
            result = InstallerTypeEnum::Msi;
        }
        else if (inStrLower == "nullsoft")
        {
            result = InstallerTypeEnum::Nullsoft;
        }
        else if (inStrLower == "zip")
        {
            result = InstallerTypeEnum::Zip;
        }
        else if (inStrLower == "appx" || inStrLower == "msix")
        {
            result = InstallerTypeEnum::Msix;
        }
        else if (inStrLower == "exe")
        {
            result = InstallerTypeEnum::Exe;
        }
        else if (inStrLower == "burn")
        {
            result = InstallerTypeEnum::Burn;
        }
        else if (inStrLower == "msstore")
        {
            result = InstallerTypeEnum::MSStore;
        }

        return result;
    }

    ManifestInstaller::UpdateBehaviorEnum ManifestInstaller::ConvertToUpdateBehaviorEnum(const std::string& in)
    {
        UpdateBehaviorEnum result = UpdateBehaviorEnum::Unknown;

        if (Utility::CaseInsensitiveEquals(in, "install"))
        {
            result = UpdateBehaviorEnum::Install;
        }
        else if (Utility::CaseInsensitiveEquals(in, "uninstallprevious"))
        {
            result = UpdateBehaviorEnum::UninstallPrevious;
        }

        return result;
    }

    std::string ManifestInstaller::InstallerTypeToString(ManifestInstaller::InstallerTypeEnum installerType)
    {
        std::string result = "Unknown";

        switch (installerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
            result = "Exe";
            break;
        case ManifestInstaller::InstallerTypeEnum::Inno:
            result = "Inno";
            break;
        case ManifestInstaller::InstallerTypeEnum::Msi:
            result = "Msi";
            break;
        case ManifestInstaller::InstallerTypeEnum::Msix:
            result = "Msix";
            break;
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
            result = "Nullsoft";
            break;
        case ManifestInstaller::InstallerTypeEnum::Wix:
            result = "Wix";
            break;
        case ManifestInstaller::InstallerTypeEnum::Zip:
            result = "Zip";
            break;
        case ManifestInstaller::InstallerTypeEnum::Burn:
            result = "Burn";
            break;
        case ManifestInstaller::InstallerTypeEnum::MSStore:
            result = "MSStore";
            break;
        default:
            break;
        }

        return result;
    }

    bool ManifestInstaller::DoesInstallerTypeUsePackageFamilyName(InstallerTypeEnum installerType)
    {
        return (installerType == InstallerTypeEnum::Msix || installerType == InstallerTypeEnum::MSStore);
    }

    bool ManifestInstaller::DoesInstallerTypeUseProductCode(InstallerTypeEnum installerType)
    {
        return (
            installerType == InstallerTypeEnum::Exe || 
            installerType == InstallerTypeEnum::Inno ||
            installerType == InstallerTypeEnum::Msi ||
            installerType == InstallerTypeEnum::Nullsoft ||
            installerType == InstallerTypeEnum::Wix ||
            installerType == InstallerTypeEnum::Burn
            );
    }

    bool ManifestInstaller::IsInstallerTypeCompatible(InstallerTypeEnum type1, InstallerTypeEnum type2)
    {
        if (type1 == InstallerTypeEnum::Unknown || type2 == InstallerTypeEnum::Unknown)
        {
            return false;
        }

        std::vector<InstallerTypeEnum> compatList1 =
        {
            InstallerTypeEnum::Exe,
            InstallerTypeEnum::Inno,
            InstallerTypeEnum::Nullsoft,
            InstallerTypeEnum::Burn,
        };

        std::vector<InstallerTypeEnum> compatList2 =
        {
            InstallerTypeEnum::Msi,
            InstallerTypeEnum::Wix
        };

        return type1 == type2 ||
            (std::find(compatList1.begin(), compatList1.end(), type1) != compatList1.end() && std::find(compatList1.begin(), compatList1.end(), type2) != compatList1.end()) ||
            (std::find(compatList2.begin(), compatList2.end(), type1) != compatList2.end() && std::find(compatList2.begin(), compatList2.end(), type2) != compatList2.end());
    }
}
