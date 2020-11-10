// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/ManifestInstaller.h"

namespace AppInstaller::Manifest
{
    namespace
    {
        enum class CompatibilitySet
        {
            None,
            Exe,
            Msi,
            Msix,
        };

        CompatibilitySet GetCompatibilitySet(ManifestInstaller::InstallerTypeEnum type)
        {
            switch (type)
            {
            case ManifestInstaller::InstallerTypeEnum::Inno:
            case ManifestInstaller::InstallerTypeEnum::Nullsoft:
            case ManifestInstaller::InstallerTypeEnum::Exe:
            case ManifestInstaller::InstallerTypeEnum::Burn:
                return CompatibilitySet::Exe;
            case ManifestInstaller::InstallerTypeEnum::Wix:
            case ManifestInstaller::InstallerTypeEnum::Msi:
                return CompatibilitySet::Msi;
            case ManifestInstaller::InstallerTypeEnum::Msix:
            case ManifestInstaller::InstallerTypeEnum::MSStore:
                return CompatibilitySet::Msix;
            default:
                return CompatibilitySet::None;
            }
        }
    }

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
        // Unknown type cannot be compatible with any other
        if (type1 == InstallerTypeEnum::Unknown || type2 == InstallerTypeEnum::Unknown)
        {
            return false;
        }

        // Not unknown, so must be compatible
        if (type1 == type2)
        {
            return true;
        }

        CompatibilitySet set1 = GetCompatibilitySet(type1);
        CompatibilitySet set2 = GetCompatibilitySet(type2);

        // If either is none, they can't be compatible
        if (set1 == CompatibilitySet::None || set2 == CompatibilitySet::None)
        {
            return false;
        }

        return set1 == set2;
    }
}
