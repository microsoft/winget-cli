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

            for (const RawVersion& ext : m_extensions)
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
        for (const RawVersion& ext : m_extensions)
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
        else if (inStrLower == "portable") 
        {
            result = InstallerTypeEnum::Portable;
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
        else if (Utility::CaseInsensitiveEquals(in, "deny"))
        {
            result = UpdateBehaviorEnum::Deny;
        }

        return result;
    }

    ScopeEnum ConvertToScopeEnum(std::string_view in)
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

    InstallModeEnum ConvertToInstallModeEnum(const std::string& in)
    {
        InstallModeEnum result = InstallModeEnum::Unknown;

        if (Utility::CaseInsensitiveEquals(in, "interactive"))
        {
            result = InstallModeEnum::Interactive;
        }
        else if (Utility::CaseInsensitiveEquals(in, "silent"))
        {
            result = InstallModeEnum::Silent;
        }
        else if (Utility::CaseInsensitiveEquals(in, "silentWithProgress"))
        {
            result = InstallModeEnum::SilentWithProgress;
        }

        return result;
    }

    PlatformEnum ConvertToPlatformEnum(std::string_view in)
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "windows.desktop")
        {
            return PlatformEnum::Desktop;
        }
        else if (inStrLower == "windows.universal")
        {
            return PlatformEnum::Universal;
        }

        return PlatformEnum::Unknown;
    }

    PlatformEnum ConvertToPlatformEnumForMSStoreDownload(std::string_view in)
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "windows.desktop")
        {
            return PlatformEnum::Desktop;
        }
        else if (inStrLower == "windows.universal")
        {
            return PlatformEnum::Universal;
        }
        else if (inStrLower == "windows.iot")
        {
            return PlatformEnum::IoT;
        }
        else if (inStrLower == "windows.team")
        {
            return PlatformEnum::Team;
        }
        else if (inStrLower == "windows.holographic")
        {
            return PlatformEnum::Holographic;
        }

        return PlatformEnum::Unknown;
    }

    ElevationRequirementEnum ConvertToElevationRequirementEnum(const std::string& in)
    {
        ElevationRequirementEnum result = ElevationRequirementEnum::Unknown;

        if (Utility::CaseInsensitiveEquals(in, "elevationRequired"))
        {
            result = ElevationRequirementEnum::ElevationRequired;
        }
        else if (Utility::CaseInsensitiveEquals(in, "elevationProhibited"))
        {
            result = ElevationRequirementEnum::ElevationProhibited;
        }
        else if (Utility::CaseInsensitiveEquals(in, "elevatesSelf"))
        {
            result = ElevationRequirementEnum::ElevatesSelf;
        }

        return result;
    }

    UnsupportedArgumentEnum ConvertToUnsupportedArgumentEnum(const std::string& in)
    {
        UnsupportedArgumentEnum result = UnsupportedArgumentEnum::Unknown;

        if (Utility::CaseInsensitiveEquals(in, "log"))
        {
            result = UnsupportedArgumentEnum::Log;
        }
        else if (Utility::CaseInsensitiveEquals(in, "location"))
        {
            result = UnsupportedArgumentEnum::Location;
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
        else if (in == "shadow")
        {
            return ManifestTypeEnum::Shadow;
        }
        else
        {
            THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), "Unsupported ManifestType: %hs", in.c_str());
        }
    }

    ExpectedReturnCodeEnum ConvertToExpectedReturnCodeEnum(const std::string& in)
    {
        std::string inStrLower = Utility::ToLower(in);
        ExpectedReturnCodeEnum result = ExpectedReturnCodeEnum::Unknown;

        if (inStrLower == "packageinuse")
        {
            result = ExpectedReturnCodeEnum::PackageInUse;
        }
        else if (inStrLower == "packageinusebyapplication")
        {
            result = ExpectedReturnCodeEnum::PackageInUseByApplication;
        }
        else if (inStrLower == "installinprogress")
        {
            result = ExpectedReturnCodeEnum::InstallInProgress;
        }
        else if (inStrLower == "fileinuse")
        {
            result = ExpectedReturnCodeEnum::FileInUse;
        }
        else if (inStrLower == "missingdependency")
        {
            result = ExpectedReturnCodeEnum::MissingDependency;
        }
        else if (inStrLower == "diskfull")
        {
            result = ExpectedReturnCodeEnum::DiskFull;
        }
        else if (inStrLower == "insufficientmemory")
        {
            result = ExpectedReturnCodeEnum::InsufficientMemory;
        }
        else if (inStrLower == "invalidparameter")
        {
            result = ExpectedReturnCodeEnum::InvalidParameter;
        }
        else if (inStrLower == "nonetwork")
        {
            result = ExpectedReturnCodeEnum::NoNetwork;
        }
        else if (inStrLower == "contactsupport")
        {
            result = ExpectedReturnCodeEnum::ContactSupport;
        }
        else if (inStrLower == "rebootrequiredtofinish")
        {
            result = ExpectedReturnCodeEnum::RebootRequiredToFinish;
        }
        else if (inStrLower == "rebootrequiredforinstall")
        {
            result = ExpectedReturnCodeEnum::RebootRequiredForInstall;
        }
        else if (inStrLower == "rebootinitiated")
        {
            result = ExpectedReturnCodeEnum::RebootInitiated;
        }
        else if (inStrLower == "cancelledbyuser")
        {
            result = ExpectedReturnCodeEnum::CancelledByUser;
        }
        else if (inStrLower == "alreadyinstalled")
        {
            result = ExpectedReturnCodeEnum::AlreadyInstalled;
        }
        else if (inStrLower == "downgrade")
        {
            result = ExpectedReturnCodeEnum::Downgrade;
        }
        else if (inStrLower == "blockedbypolicy")
        {
            result = ExpectedReturnCodeEnum::BlockedByPolicy;
        }
        else if (inStrLower == "systemnotsupported")
        {
            result = ExpectedReturnCodeEnum::SystemNotSupported;
        }
        else if (inStrLower == "custom")
        {
            result = ExpectedReturnCodeEnum::Custom;
        }

        return result;
    }

    InstalledFileTypeEnum ConvertToInstalledFileTypeEnum(const std::string& in)
    {
        std::string inStrLower = Utility::ToLower(in);
        InstalledFileTypeEnum result = InstalledFileTypeEnum::Unknown;

        if (inStrLower == "launch")
        {
            result = InstalledFileTypeEnum::Launch;
        }
        else if (inStrLower == "uninstall")
        {
            result = InstalledFileTypeEnum::Uninstall;
        }
        else if (inStrLower == "other")
        {
            result = InstalledFileTypeEnum::Other;
        }

        return result;
    }

    IconFileTypeEnum ConvertToIconFileTypeEnum(std::string_view in)
    {
        std::string inStrLower = Utility::ToLower(in);
        IconFileTypeEnum result = IconFileTypeEnum::Unknown;

        if (inStrLower == "jpeg")
        {
            result = IconFileTypeEnum::Jpeg;
        }
        else if (inStrLower == "png")
        {
            result = IconFileTypeEnum::Png;
        }
        else if (inStrLower == "ico")
        {
            result = IconFileTypeEnum::Ico;
        }

        return result;
    }

    IconThemeEnum ConvertToIconThemeEnum(std::string_view in)
    {
        std::string inStrLower = Utility::ToLower(in);
        IconThemeEnum result = IconThemeEnum::Unknown;

        if (inStrLower == "default")
        {
            result = IconThemeEnum::Default;
        }
        else if (inStrLower == "dark")
        {
            result = IconThemeEnum::Dark;
        }
        else if (inStrLower == "light")
        {
            result = IconThemeEnum::Light;
        }
        else if (inStrLower == "highcontrast")
        {
            result = IconThemeEnum::HighContrast;
        }

        return result;
    }

    IconResolutionEnum ConvertToIconResolutionEnum(std::string_view in)
    {
        std::string inStrLower = Utility::ToLower(in);
        IconResolutionEnum result = IconResolutionEnum::Unknown;

        if (inStrLower == "custom")
        {
            result = IconResolutionEnum::Custom;
        }
        else if (inStrLower == "16x16")
        {
            result = IconResolutionEnum::Square16;
        }
        else if (inStrLower == "20x20")
        {
            result = IconResolutionEnum::Square20;
        }
        else if (inStrLower == "24x24")
        {
            result = IconResolutionEnum::Square24;
        }
        else if (inStrLower == "30x30")
        {
            result = IconResolutionEnum::Square30;
        }
        else if (inStrLower == "32x32")
        {
            result = IconResolutionEnum::Square32;
        }
        else if (inStrLower == "36x36")
        {
            result = IconResolutionEnum::Square36;
        }
        else if (inStrLower == "40x40")
        {
            result = IconResolutionEnum::Square40;
        }
        else if (inStrLower == "48x48")
        {
            result = IconResolutionEnum::Square48;
        }
        else if (inStrLower == "60x60")
        {
            result = IconResolutionEnum::Square60;
        }
        else if (inStrLower == "64x64")
        {
            result = IconResolutionEnum::Square64;
        }
        else if (inStrLower == "72x72")
        {
            result = IconResolutionEnum::Square72;
        }
        else if (inStrLower == "80x80")
        {
            result = IconResolutionEnum::Square80;
        }
        else if (inStrLower == "96x96")
        {
            result = IconResolutionEnum::Square96;
        }
        else if (inStrLower == "256x256")
        {
            result = IconResolutionEnum::Square256;
        }

        return result;
    }

    std::string_view InstallerTypeToString(InstallerTypeEnum installerType)
    {
        switch (installerType)
        {
        case InstallerTypeEnum::Exe:
            return "exe"sv;
        case InstallerTypeEnum::Inno:
            return "inno"sv;
        case InstallerTypeEnum::Msi:
            return "msi"sv;
        case InstallerTypeEnum::Msix:
            return "msix"sv;
        case InstallerTypeEnum::Nullsoft:
            return "nullsoft"sv;
        case InstallerTypeEnum::Wix:
            return "wix"sv;
        case InstallerTypeEnum::Zip:
            return "zip"sv;
        case InstallerTypeEnum::Burn:
            return "burn"sv;
        case InstallerTypeEnum::MSStore:
            return "msstore"sv;
        case InstallerTypeEnum::Portable:
            return "portable"sv;
        }

        return "unknown"sv;
    }

    std::string_view InstallerSwitchTypeToString(InstallerSwitchType installerSwitchType)
    {
        switch (installerSwitchType)
        {
        case InstallerSwitchType::Custom:
            return "Custom"sv;
        case InstallerSwitchType::Silent:
            return "Silent"sv;
        case InstallerSwitchType::SilentWithProgress:
            return "SilentWithProgress"sv;
        case InstallerSwitchType::Interactive:
            return "Interactive"sv;
        case InstallerSwitchType::Language:
            return "Language"sv;
        case InstallerSwitchType::Log:
            return "Log"sv;
        case InstallerSwitchType::InstallLocation:
            return "InstallLocation"sv;
        case InstallerSwitchType::Update:
            return "Upgrade"sv;
        case InstallerSwitchType::Repair:
            return "Repair"sv;
        }

        return "Unknown"sv;
    }

    std::string_view ElevationRequirementToString(ElevationRequirementEnum elevationRequirement)
    {
        switch (elevationRequirement)
        {
        case ElevationRequirementEnum::ElevationRequired:
            return "elevationRequired"sv;
        case ElevationRequirementEnum::ElevationProhibited:
            return "elevationProhibited"sv;
        case ElevationRequirementEnum::ElevatesSelf:
            return "elevatesSelf"sv;
        }

        return "unknown"sv;
    }

    std::string_view UnsupportedArgumentToString(UnsupportedArgumentEnum unsupportedArgument)
    {
        switch (unsupportedArgument)
        {
        case UnsupportedArgumentEnum::Log:
            return "log"sv;
        case UnsupportedArgumentEnum::Location:
            return "location"sv;
        }

        return "unknown"sv;
    }

    std::string_view InstallModeToString(InstallModeEnum installMode)
    {
        switch (installMode)
        {
        case InstallModeEnum::Interactive:
            return "interactive"sv;
        case InstallModeEnum::Silent:
            return "silent"sv;
        case InstallModeEnum::SilentWithProgress:
            return "silentWithProgress"sv;
        }

        return "unknown"sv;
    }

    std::string_view PlatformToString(PlatformEnum platform, bool shortString)
    {
        switch (platform)
        {
        case PlatformEnum::Desktop:
            return shortString ? "Desktop" : "Windows.Desktop"sv;
        case PlatformEnum::Universal:
            return shortString ? "Universal" : "Windows.Universal"sv;
        case PlatformEnum::IoT:
            return shortString ? "IoT" : "Windows.IoT"sv;
        case PlatformEnum::Holographic:
            return shortString ? "Holographic" : "Windows.Holographic"sv;
        case PlatformEnum::Team:
            return shortString ? "Team" : "Windows.Team"sv;
        }

        return "Unknown"sv;
    }

    std::string_view UpdateBehaviorToString(UpdateBehaviorEnum updateBehavior)
    {
        switch (updateBehavior)
        {
        case UpdateBehaviorEnum::Install:
            return "install"sv;
        case UpdateBehaviorEnum::UninstallPrevious:
            return "uninstallPrevious"sv;
        case UpdateBehaviorEnum::Deny:
            return "deny"sv;
        }

        return "unknown"sv;
    }

    std::string_view RepairBehaviorToString(RepairBehaviorEnum repairBehavior)
    {
        switch (repairBehavior)
        {
        case AppInstaller::Manifest::RepairBehaviorEnum::Modify:
            return "modify"sv;
        case AppInstaller::Manifest::RepairBehaviorEnum::Installer:
            return "installer"sv;
        case AppInstaller::Manifest::RepairBehaviorEnum::Uninstaller:
            return "uninstaller"sv;
        }

        return "unknown"sv;
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

    std::string_view InstalledFileTypeToString(InstalledFileTypeEnum installedFileType)
    {
        switch (installedFileType)
        {
        case InstalledFileTypeEnum::Launch:
            return "launch"sv;
        case InstalledFileTypeEnum::Uninstall:
            return "uninstall"sv;
        case InstalledFileTypeEnum::Other:
            return "other"sv;
        }

        return "unknown"sv;
    }

    std::string_view IconFileTypeToString(IconFileTypeEnum iconFileType)
    {
        switch (iconFileType)
        {
        case IconFileTypeEnum::Ico:
            return "ico"sv;
        case IconFileTypeEnum::Jpeg:
            return "jpeg"sv;
        case IconFileTypeEnum::Png:
            return "png"sv;
        }

        return "unknown"sv;
    }

    std::string_view IconThemeToString(IconThemeEnum iconTheme)
    {
        switch (iconTheme)
        {
        case IconThemeEnum::Default:
            return "default"sv;
        case IconThemeEnum::Dark:
            return "dark"sv;
        case IconThemeEnum::Light:
            return "light"sv;
        case IconThemeEnum::HighContrast:
            return "highContrast"sv;
        }

        return "unknown"sv;
    }

    std::string_view IconResolutionToString(IconResolutionEnum iconResolution)
    {
        switch (iconResolution)
        {
        case IconResolutionEnum::Custom:
            return "custom"sv;
        case IconResolutionEnum::Square16:
            return "16x16"sv;
        case IconResolutionEnum::Square20:
            return "20x20"sv;
        case IconResolutionEnum::Square24:
            return "24x24"sv;
        case IconResolutionEnum::Square30:
            return "30x30"sv;
        case IconResolutionEnum::Square32:
            return "32x32"sv;
        case IconResolutionEnum::Square36:
            return "36x36"sv;
        case IconResolutionEnum::Square40:
            return "40x40"sv;
        case IconResolutionEnum::Square48:
            return "48x48"sv;
        case IconResolutionEnum::Square60:
            return "60x60"sv;
        case IconResolutionEnum::Square64:
            return "64x64"sv;
        case IconResolutionEnum::Square72:
            return "72x72"sv;
        case IconResolutionEnum::Square80:
            return "80x80"sv;
        case IconResolutionEnum::Square96:
            return "96x96"sv;
        case IconResolutionEnum::Square256:
            return "256x256"sv;
        }

        return "unknown"sv;
    }

    std::string_view ExpectedReturnCodeToString(ExpectedReturnCodeEnum expectedReturnCode)
    {
        switch (expectedReturnCode)
        {
        case ExpectedReturnCodeEnum::AlreadyInstalled:
            return "alreadyInstalled"sv;
        case ExpectedReturnCodeEnum::PackageInUse:
            return "packageInUse"sv;
        case ExpectedReturnCodeEnum::PackageInUseByApplication:
            return "packageInUseByApplication"sv;
        case ExpectedReturnCodeEnum::InstallInProgress:
            return "installInProgress"sv;
        case ExpectedReturnCodeEnum::FileInUse:
            return "fileInUse"sv;
        case ExpectedReturnCodeEnum::MissingDependency:
            return "missingDependency"sv;
        case ExpectedReturnCodeEnum::DiskFull:
            return "diskFull"sv;
        case ExpectedReturnCodeEnum::InsufficientMemory:
            return "insufficientMemory"sv;
        case ExpectedReturnCodeEnum::InvalidParameter:
            return "invalidParameter"sv;
        case ExpectedReturnCodeEnum::NoNetwork:
            return "noNetwork"sv;
        case ExpectedReturnCodeEnum::ContactSupport:
            return "contactSupport"sv;
        case ExpectedReturnCodeEnum::RebootRequiredToFinish:
            return "rebootRequiredToFinish"sv;
        case ExpectedReturnCodeEnum::RebootRequiredForInstall:
            return "rebootRequiredForInstall"sv;
        case ExpectedReturnCodeEnum::RebootInitiated:
            return "rebootInitiated"sv;
        case ExpectedReturnCodeEnum::CancelledByUser:
            return "cancelledByUser"sv;
        case ExpectedReturnCodeEnum::Downgrade:
            return "downgrade"sv;
        case ExpectedReturnCodeEnum::BlockedByPolicy:
            return "blockedByPolicy"sv;
        case ExpectedReturnCodeEnum::SystemNotSupported:
            return "systemNotSupported"sv;
        case ExpectedReturnCodeEnum::Custom:
            return "custom"sv;
        }

        return "unknown"sv;
    }

    std::string_view ManifestTypeToString(ManifestTypeEnum manifestType)
    {
        switch (manifestType)
        {
        case ManifestTypeEnum::DefaultLocale:
            return "defaultLocale"sv;
        case ManifestTypeEnum::Installer:
            return "installer"sv;
        case ManifestTypeEnum::Locale:
            return "locale"sv;
        case ManifestTypeEnum::Merged:
            return "merged"sv;
        case ManifestTypeEnum::Singleton:
            return "singleton"sv;
        case ManifestTypeEnum::Version:
            return "version"sv;
        }

        return "unknown"sv;
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
            installerType == InstallerTypeEnum::Burn ||
            installerType == InstallerTypeEnum::Portable
            );
    }

    bool DoesInstallerTypeWriteAppsAndFeaturesEntry(InstallerTypeEnum installerType)
    {
        return (
            installerType == InstallerTypeEnum::Exe ||
            installerType == InstallerTypeEnum::Inno ||
            installerType == InstallerTypeEnum::Msi ||
            installerType == InstallerTypeEnum::Nullsoft ||
            installerType == InstallerTypeEnum::Wix ||
            installerType == InstallerTypeEnum::Burn ||
            installerType == InstallerTypeEnum::Portable
            );
    }

    bool DoesInstallerTypeSupportArpVersionRange(InstallerTypeEnum installerType)
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

    bool DoesInstallerTypeIgnoreScopeFromManifest(InstallerTypeEnum installerType)
    {
        return
            installerType == InstallerTypeEnum::Portable ||
            installerType == InstallerTypeEnum::Msix ||
            installerType == InstallerTypeEnum::MSStore;
    }

    bool DoesInstallerTypeRequireAdminForMachineScopeInstall(InstallerTypeEnum installerType)
    {
        return
            installerType == InstallerTypeEnum::Portable ||
            installerType == InstallerTypeEnum::MSStore ||
            installerType == InstallerTypeEnum::Msix;
    }

    bool DoesInstallerTypeRequireRepairBehaviorForRepair(InstallerTypeEnum installerType)
    {
        return
            installerType == InstallerTypeEnum::Burn ||
            installerType == InstallerTypeEnum::Inno ||
            installerType == InstallerTypeEnum::Nullsoft ||
            installerType == InstallerTypeEnum::Exe;
    }

    bool IsArchiveType(InstallerTypeEnum installerType)
    {
        return (installerType == InstallerTypeEnum::Zip);
    }

    bool IsPortableType(InstallerTypeEnum installerType)
    {
        return (installerType == InstallerTypeEnum::Portable);
    }

    bool IsNestedInstallerTypeSupported(InstallerTypeEnum nestedInstallerType)
    {
        return (
            nestedInstallerType == InstallerTypeEnum::Exe ||
            nestedInstallerType == InstallerTypeEnum::Inno ||
            nestedInstallerType == InstallerTypeEnum::Msi ||
            nestedInstallerType == InstallerTypeEnum::Nullsoft ||
            nestedInstallerType == InstallerTypeEnum::Wix ||
            nestedInstallerType == InstallerTypeEnum::Burn ||
            nestedInstallerType == InstallerTypeEnum::Portable ||
            nestedInstallerType == InstallerTypeEnum::Msix
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
                {InstallerSwitchType::Silent, ManifestInstaller::string_t("/quiet /norestart")},
                {InstallerSwitchType::SilentWithProgress, ManifestInstaller::string_t("/passive /norestart")},
                {InstallerSwitchType::Log, ManifestInstaller::string_t("/log \"" + std::string(ARG_TOKEN_LOGPATH) + "\"")},
                {InstallerSwitchType::InstallLocation, ManifestInstaller::string_t("TARGETDIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"")}
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
                {InstallerSwitchType::Silent, ManifestInstaller::string_t("/SP- /VERYSILENT /SUPPRESSMSGBOXES /NORESTART")},
                {InstallerSwitchType::SilentWithProgress, ManifestInstaller::string_t("/SP- /SILENT /SUPPRESSMSGBOXES /NORESTART")},
                {InstallerSwitchType::Log, ManifestInstaller::string_t("/LOG=\"" + std::string(ARG_TOKEN_LOGPATH) + "\"")},
                {InstallerSwitchType::InstallLocation, ManifestInstaller::string_t("/DIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"")}
            };
        default:
            return {};
        }
    }

    RepairBehaviorEnum ConvertToRepairBehaviorEnum(std::string_view in)
    {
        std::string inStrLower = Utility::ToLower(in);
        RepairBehaviorEnum result = RepairBehaviorEnum::Unknown;

        if (inStrLower == "installer")
        {
            result = RepairBehaviorEnum::Installer;
        }
        else if (inStrLower == "uninstaller")
        {
            result = RepairBehaviorEnum::Uninstaller;
        }
        else if (inStrLower == "modify")
        {
            result = RepairBehaviorEnum::Modify;
        }

        return result;
    }

    std::map<DWORD, ExpectedReturnCodeEnum> GetDefaultKnownReturnCodes(InstallerTypeEnum installerType)
    {
        switch (installerType)
        {
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Wix:
        case InstallerTypeEnum::Msi:
            // See https://docs.microsoft.com/windows/win32/msi/error-codes
            return
            {
                { ERROR_INSTALL_ALREADY_RUNNING, ExpectedReturnCodeEnum::InstallInProgress },
                { ERROR_DISK_FULL, ExpectedReturnCodeEnum::DiskFull },
                { ERROR_INSTALL_SERVICE_FAILURE, ExpectedReturnCodeEnum::ContactSupport },
                { ERROR_SUCCESS_REBOOT_REQUIRED, ExpectedReturnCodeEnum::RebootRequiredToFinish },
                { ERROR_SUCCESS_REBOOT_INITIATED, ExpectedReturnCodeEnum::RebootInitiated },
                { ERROR_INSTALL_USEREXIT, ExpectedReturnCodeEnum::CancelledByUser },
                { ERROR_PRODUCT_VERSION, ExpectedReturnCodeEnum::AlreadyInstalled },
                { ERROR_INSTALL_REJECTED, ExpectedReturnCodeEnum::SystemNotSupported },
                { ERROR_INSTALL_PACKAGE_REJECTED, ExpectedReturnCodeEnum::BlockedByPolicy },
                { ERROR_INSTALL_TRANSFORM_REJECTED, ExpectedReturnCodeEnum::BlockedByPolicy },
                { ERROR_PATCH_PACKAGE_REJECTED, ExpectedReturnCodeEnum::BlockedByPolicy },
                { ERROR_PATCH_REMOVAL_DISALLOWED, ExpectedReturnCodeEnum::BlockedByPolicy },
                { ERROR_INSTALL_REMOTE_DISALLOWED, ExpectedReturnCodeEnum::BlockedByPolicy },
                { ERROR_INVALID_PARAMETER, ExpectedReturnCodeEnum::InvalidParameter },
                { ERROR_INVALID_TABLE, ExpectedReturnCodeEnum::InvalidParameter },
                { ERROR_INVALID_COMMAND_LINE, ExpectedReturnCodeEnum::InvalidParameter },
                { ERROR_INVALID_PATCH_XML, ExpectedReturnCodeEnum::InvalidParameter },
                { ERROR_INSTALL_LANGUAGE_UNSUPPORTED, ExpectedReturnCodeEnum::SystemNotSupported },
                { ERROR_INSTALL_PLATFORM_UNSUPPORTED, ExpectedReturnCodeEnum::SystemNotSupported },
            };
        case InstallerTypeEnum::Inno:
            // See https://jrsoftware.org/ishelp/index.php?topic=setupexitcodes
            return
            {
                { 2, ExpectedReturnCodeEnum::CancelledByUser },
                { 5, ExpectedReturnCodeEnum::CancelledByUser },
                { 8, ExpectedReturnCodeEnum::RebootRequiredForInstall },
            };
        case InstallerTypeEnum::Msix:
            // See https://docs.microsoft.com/en-us/windows/win32/appxpkg/troubleshooting
            return
            {
                { HRESULT_FROM_WIN32(ERROR_INSTALL_PREREQUISITE_FAILED), ExpectedReturnCodeEnum::MissingDependency },
                { HRESULT_FROM_WIN32(ERROR_INSTALL_RESOLVE_DEPENDENCY_FAILED), ExpectedReturnCodeEnum::MissingDependency },
                { HRESULT_FROM_WIN32(ERROR_INSTALL_OPTIONAL_PACKAGE_REQUIRES_MAIN_PACKAGE), ExpectedReturnCodeEnum::MissingDependency },
                { HRESULT_FROM_WIN32(ERROR_INSTALL_OUT_OF_DISK_SPACE), ExpectedReturnCodeEnum::DiskFull },
                { HRESULT_FROM_WIN32(ERROR_INSTALL_CANCEL), ExpectedReturnCodeEnum::CancelledByUser },
                { HRESULT_FROM_WIN32(ERROR_PACKAGE_ALREADY_EXISTS), ExpectedReturnCodeEnum::AlreadyInstalled },
                { HRESULT_FROM_WIN32(ERROR_INSTALL_PACKAGE_DOWNGRADE), ExpectedReturnCodeEnum::Downgrade },
                { HRESULT_FROM_WIN32(ERROR_DEPLOYMENT_BLOCKED_BY_POLICY), ExpectedReturnCodeEnum::BlockedByPolicy},
                { HRESULT_FROM_WIN32(ERROR_INSTALL_POLICY_FAILURE), ExpectedReturnCodeEnum::BlockedByPolicy},
                { HRESULT_FROM_WIN32(ERROR_PACKAGES_IN_USE), ExpectedReturnCodeEnum::PackageInUse },
                { HRESULT_FROM_WIN32(ERROR_INSTALL_WRONG_PROCESSOR_ARCHITECTURE), ExpectedReturnCodeEnum::SystemNotSupported },
                { HRESULT_FROM_WIN32(ERROR_PACKAGE_NOT_SUPPORTED_ON_FILESYSTEM), ExpectedReturnCodeEnum::SystemNotSupported },
                { HRESULT_FROM_WIN32(ERROR_DEPLOYMENT_OPTION_NOT_SUPPORTED), ExpectedReturnCodeEnum::SystemNotSupported },
                { HRESULT_FROM_WIN32(ERROR_PACKAGE_LACKS_CAPABILITY_TO_DEPLOY_ON_HOST), ExpectedReturnCodeEnum::SystemNotSupported },
            };
        default:
            return {};
        }
    }

    void DependencyList::Add(const Dependency& newDependency)
    {
        Dependency* existingDependency = this->HasDependency(newDependency);

        if (existingDependency != NULL)
        {
            if (newDependency.MinVersion > existingDependency->MinVersion)
            {
                existingDependency->MinVersion = newDependency.MinVersion;
            }
        }
        else
        {
            m_dependencies.push_back(newDependency);
        }
    }

    void DependencyList::Add(const DependencyList& otherDependencyList)
    {
        for (const auto& dependency : otherDependencyList.m_dependencies)
        {
            this->Add(dependency);
        }
    }

    bool DependencyList::HasAny() const { return !m_dependencies.empty(); }
    bool DependencyList::HasAnyOf(DependencyType type) const
    {
        for (const auto& dependency : m_dependencies)
        {
            if (dependency.Type == type) return true;
        };
        return false;
    }

    Dependency* DependencyList::HasDependency(const Dependency& dependencyToSearch)
    {
        for (auto& dependency : m_dependencies)
        {
            if (dependency.Type == dependencyToSearch.Type && ICUCaseInsensitiveEquals(dependency.Id(), dependencyToSearch.Id()))
            {
                return &dependency;
            }
        }
        return nullptr;
    }

    // for testing purposes
    bool DependencyList::HasExactDependency(DependencyType type, const string_t& id, const string_t& minVersion)
    {
        for (const auto& dependency : m_dependencies)
        {
            if (dependency.Type == type && Utility::ICUCaseInsensitiveEquals(dependency.Id(), id))
            {
                if (!minVersion.empty())
                {
                    return dependency.MinVersion == Utility::Version{ minVersion };
                }
                else
                {
                    return true;
                }
            }
        }
        return false;
    }

    size_t DependencyList::Size()
    {
        return m_dependencies.size();
    }

    void DependencyList::ApplyToType(DependencyType type, std::function<void(const Dependency&)> func) const
    {
        for (const auto& dependency : m_dependencies)
        {
            if (dependency.Type == type) func(dependency);
        }
    }

    void DependencyList::ApplyToAll(std::function<void(const Dependency&)> func) const
    {
        for (const auto& dependency : m_dependencies)
        {
            func(dependency);
        }
    }

    bool DependencyList::Empty() const
    {
        return m_dependencies.empty();
    }

    void DependencyList::Clear() { m_dependencies.clear(); }
}
