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
        else if (inStrLower == "custom")
        {
            result = ExpectedReturnCodeEnum::Custom;
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

    bool DoesInstallerTypeWriteAppsAndFeaturesEntry(InstallerTypeEnum installerType)
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
                {InstallerSwitchType::Silent, ManifestInstaller::string_t("/VERYSILENT")},
                {InstallerSwitchType::SilentWithProgress, ManifestInstaller::string_t("/SILENT")},
                {InstallerSwitchType::Log, ManifestInstaller::string_t("/LOG=\"" + std::string(ARG_TOKEN_LOGPATH) + "\"")},
                {InstallerSwitchType::InstallLocation, ManifestInstaller::string_t("/DIR=\"" + std::string(ARG_TOKEN_INSTALLPATH) + "\"")}
            };
        default:
            return {};
        }
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
                { ERROR_INSTALL_REJECTED, ExpectedReturnCodeEnum::BlockedByPolicy },
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
            };
        default:
            return {};
        }
    }

    void DependencyList::Add(const Dependency& newDependency)
    {
        Dependency* existingDependency = this->HasDependency(newDependency);

        if (existingDependency != NULL) {
            if (newDependency.MinVersion)
            {
                if (existingDependency->MinVersion)
                {
                    const auto& newDependencyVersion = Utility::Version(newDependency.MinVersion.value());
                    const auto& existingDependencyVersion = Utility::Version(existingDependency->MinVersion.value());
                    if (newDependencyVersion > existingDependencyVersion)
                    {
                        existingDependency->MinVersion.value() = newDependencyVersion.ToString();
                    }
                }
                else
                {
                    existingDependency->MinVersion.value() = newDependency.MinVersion.value();
                }
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
        for (auto& dependency : m_dependencies) {
            if (dependency.Type == dependencyToSearch.Type && ICUCaseInsensitiveEquals(dependency.Id, dependencyToSearch.Id))
            {
                return &dependency;
            }
        }
        return nullptr;
    }

    // for testing purposes
    bool DependencyList::HasExactDependency(DependencyType type, string_t id, string_t minVersion)
    {
        for (const auto& dependency : m_dependencies)
        {
            if (dependency.Type == type && Utility::ICUCaseInsensitiveEquals(dependency.Id, id))
            {
                if (dependency.MinVersion) {
                    if (dependency.MinVersion.value() == minVersion)
                    {
                        return true;
                    }
                }
                else {
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
