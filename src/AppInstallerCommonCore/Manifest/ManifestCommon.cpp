// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/ManifestCommon.h"
#include "winget/ManifestValidation.h"

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

        CompatibilitySet GetCompatibilitySet(InstallerTypeEnum type)
        {
            switch (type)
            {
            case InstallerTypeEnum::Inno:
            case InstallerTypeEnum::Nullsoft:
            case InstallerTypeEnum::Exe:
            case InstallerTypeEnum::Burn:
                return CompatibilitySet::Exe;
            case InstallerTypeEnum::Wix:
            case InstallerTypeEnum::Msi:
                return CompatibilitySet::Msi;
            case InstallerTypeEnum::Msix:
            case InstallerTypeEnum::MSStore:
                return CompatibilitySet::Msix;
            default:
                return CompatibilitySet::None;
            }
        }
    }

    ManifestVer::ManifestVer(std::string_view version)
    {
        bool validationSuccess = true;

        // Separate the extensions out
        size_t hyphenPos = version.find_first_of('-');
        if (hyphenPos != std::string_view::npos)
        {
            // The first part is the main version
            Assign(std::string{ version.substr(0, hyphenPos) }, ".");

            // The second part is the extensions
            hyphenPos += 1;
            while (hyphenPos < version.length())
            {
                size_t newPos = version.find_first_of('-', hyphenPos);

                size_t length = (newPos == std::string::npos ? version.length() : newPos) - hyphenPos;
                m_extensions.emplace_back(std::string{ version.substr(hyphenPos, length) }, ".");

                hyphenPos += length + 1;
            }
        }
        else
        {
            Assign(std::string{ version }, ".");
        }

        if (m_parts.size() > 3)
        {
            validationSuccess = false;
        }
        else
        {
            for (size_t i = 0; i < m_parts.size(); i++)
            {
                if (!m_parts[i].Other.empty())
                {
                    validationSuccess = false;
                    break;
                }
            }

            for (const Version& ext : m_extensions)
            {
                if (ext.GetParts().empty() || ext.GetParts()[0].Integer != 0)
                {
                    validationSuccess = false;
                    break;
                }
            }
        }

        if (!validationSuccess)
        {
            std::vector<ValidationError> errors;
            errors.emplace_back(ManifestError::InvalidFieldValue, "ManifestVersion", std::string{ version });
            THROW_EXCEPTION(ManifestException(std::move(errors)));
        }
    }

    bool ManifestVer::HasExtension() const
    {
        return !m_extensions.empty();
    }

    bool ManifestVer::HasExtension(std::string_view extension) const
    {
        for (const Version& ext : m_extensions)
        {
            const auto& parts = ext.GetParts();
            if (!parts.empty() && parts[0].Integer == 0 && parts[0].Other == extension)
            {
                return true;
            }
        }

        return false;
    }

    InstallerTypeEnum ConvertToInstallerTypeEnum(const std::string& in)
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

    UpdateBehaviorEnum ConvertToUpdateBehaviorEnum(const std::string& in)
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

    ScopeEnum ConvertToScopeEnum(const std::string& in)
    {
        ScopeEnum result = ScopeEnum::Unknown;

        if (Utility::CaseInsensitiveEquals(in, "user"))
        {
            result = ScopeEnum::User;
        }
        else if (Utility::CaseInsensitiveEquals(in, "machine"))
        {
            result = ScopeEnum::Machine;
        }

        return result;
    }

    PlatformEnum ConvertToPlatformEnum(const std::string& in)
    {
        PlatformEnum result = PlatformEnum::Unknown;

        if (Utility::CaseInsensitiveEquals(in, "windows.desktop"))
        {
            result = PlatformEnum::Desktop;
        }
        else if (Utility::CaseInsensitiveEquals(in, "windows.universal"))
        {
            result = PlatformEnum::Universal;
        }

        return result;
    }

    ManifestTypeEnum ConvertToManifestTypeEnum(const std::string& in)
    {
        if (in == "singleton")
        {
            return ManifestTypeEnum::Singleton;
        }
        else if (in == "version")
        {
            return ManifestTypeEnum::Version;
        }
        else if (in == "installer")
        {
            return ManifestTypeEnum::Installer;
        }
        else if (in == "defaultLocale")
        {
            return ManifestTypeEnum::DefaultLocale;
        }
        else if (in == "locale")
        {
            return ManifestTypeEnum::Locale;
        }
        else if (in == "merged")
        {
            return ManifestTypeEnum::Merged;
        }
        else
        {
            THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), "Unsupported ManifestType: %s", in.c_str());
        }
    }

    std::string_view InstallerTypeToString(InstallerTypeEnum installerType)
    {
        switch (installerType)
        {
        case InstallerTypeEnum::Exe:
            return "Exe"sv;
        case InstallerTypeEnum::Inno:
            return "Inno"sv;
        case InstallerTypeEnum::Msi:
            return "Msi"sv;
        case InstallerTypeEnum::Msix:
            return "Msix"sv;
        case InstallerTypeEnum::Nullsoft:
            return "Nullsoft"sv;
        case InstallerTypeEnum::Wix:
            return "Wix"sv;
        case InstallerTypeEnum::Zip:
            return "Zip"sv;
        case InstallerTypeEnum::Burn:
            return "Burn"sv;
        case InstallerTypeEnum::MSStore:
            return "MSStore"sv;
        }

        return "Unknown"sv;
    }

    std::string_view ScopeToString(ScopeEnum scope)
    {
        switch (scope)
        {
        case ScopeEnum::User:
            return "User"sv;
        case ScopeEnum::Machine:
            return "Machine"sv;
        }

        return "Unknown"sv;
    }

    bool DoesInstallerTypeUsePackageFamilyName(InstallerTypeEnum installerType)
    {
        return (installerType == InstallerTypeEnum::Msix || installerType == InstallerTypeEnum::MSStore);
    }

    bool DoesInstallerTypeUseProductCode(InstallerTypeEnum installerType)
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

    bool IsInstallerTypeCompatible(InstallerTypeEnum type1, InstallerTypeEnum type2)
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

    std::map<InstallerSwitchType, Utility::NormalizedString> GetDefaultKnownSwitches(InstallerTypeEnum installerType)
    {
        switch (installerType)
        {
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Wix:
        case InstallerTypeEnum::Msi:
            return
            {
                {InstallerSwitchType::Silent, ManifestInstaller::string_t("/quiet")},
                {InstallerSwitchType::SilentWithProgress, ManifestInstaller::string_t("/passive")},
                {InstallerSwitchType::Log, ManifestInstaller::string_t("/log \"" + std::string(ARG_TOKEN_LOGPATH) + "\"")},
                {InstallerSwitchType::InstallLocation, ManifestInstaller::string_t("TARGETDIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"")},
                {InstallerSwitchType::Update, ManifestInstaller::string_t("REINSTALL=ALL REINSTALLMODE=vamus")}
            };
        case InstallerTypeEnum::Nullsoft:
            return
            {
                {InstallerSwitchType::Silent, ManifestInstaller::string_t("/S")},
                {InstallerSwitchType::SilentWithProgress, ManifestInstaller::string_t("/S")},
                {InstallerSwitchType::InstallLocation, ManifestInstaller::string_t("/D=" + std::string(ARG_TOKEN_INSTALLPATH))}
            };
        case InstallerTypeEnum::Inno:
            return
            {
                {InstallerSwitchType::Silent, ManifestInstaller::string_t("/VERYSILENT")},
                {InstallerSwitchType::SilentWithProgress, ManifestInstaller::string_t("/SILENT")},
                {InstallerSwitchType::Log, ManifestInstaller::string_t("/LOG=\"" + std::string(ARG_TOKEN_LOGPATH) + "\"")},
                {InstallerSwitchType::InstallLocation, ManifestInstaller::string_t("/DIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"")}
            };
        default:
            return {};
        }
    }
}
