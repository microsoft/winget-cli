// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestComparator.h"
#include "WorkflowBase.h"
#include <AppInstallerLogging.h>
#include <winget/UserSettings.h>
#include <winget/Runtime.h>
#include <winget/Locale.h>

using namespace AppInstaller::CLI;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Manifest
{
    std::ostream& operator<<(std::ostream& out, const AppInstaller::Manifest::ManifestInstaller& installer)
    {
        return out << '[' <<
            AppInstaller::Utility::ToString(installer.Arch) << ',' <<
            AppInstaller::Manifest::InstallerTypeToString(installer.EffectiveInstallerType()) << ',' <<
            AppInstaller::Manifest::ScopeToString(installer.Scope) << ',' <<
            installer.Locale << ']';
    }
}

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        struct PortableInstallFilter : public details::FilterField
        {
            PortableInstallFilter() : details::FilterField("Portable Install") {}

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                // Unvirtualized resources restricted capability is only supported for >= 10.0.18362
                // TODO: Add support for OS versions that don't support virtualization.
                if (installer.EffectiveInstallerType() == InstallerTypeEnum::Portable && !Runtime::IsCurrentOSVersionGreaterThanOrEqual(Utility::Version("10.0.18362")))
                {
                    return InapplicabilityFlags::OSVersion;
                }

                return InapplicabilityFlags::None;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller&) override
            {
                std::string result = "Current OS is lower than supported MinOSVersion (10.0.18362) for Portable install";
                return result;
            }
        };

        struct OSVersionFilter : public details::FilterField
        {
            OSVersionFilter() : details::FilterField("OS Version") {}

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                if (installer.MinOSVersion.empty() || Runtime::IsCurrentOSVersionGreaterThanOrEqual(Utility::Version(installer.MinOSVersion)))
                {
                    return InapplicabilityFlags::None;
                }

                return InapplicabilityFlags::OSVersion;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Current OS is lower than MinOSVersion ";
                result += installer.MinOSVersion;
                return result;
            }
        };

        struct MachineArchitectureComparator : public details::ComparisonField
        {
            MachineArchitectureComparator() : details::ComparisonField("Machine Architecture") {}

            MachineArchitectureComparator(std::vector<Utility::Architecture> allowedArchitectures) :
                details::ComparisonField("Machine Architecture"), m_allowedArchitectures(std::move(allowedArchitectures))
            {
                AICLI_LOG(CLI, Verbose, << "Architecture Comparator created with allowed architectures: " << Utility::ConvertContainerToString(m_allowedArchitectures, Utility::ToString));
            }

            static std::unique_ptr<MachineArchitectureComparator> Create(const Execution::Context& context, const Repository::IPackageVersion::Metadata& metadata)
            {
                std::vector<Utility::Architecture> allowedArchitectures;
                bool skipApplicabilityCheck = false;

                if (context.Contains(Execution::Data::AllowedArchitectures))
                {
                    // Com caller can directly set allowed architectures
                    allowedArchitectures = context.Get<Execution::Data::AllowedArchitectures>();
                }
                else if (context.Args.Contains(Execution::Args::Type::InstallArchitecture))
                {
                    // Arguments provided in command line
                    allowedArchitectures.emplace_back(Utility::ConvertToArchitectureEnum(context.Args.GetArg(Execution::Args::Type::InstallArchitecture)));
                }
                else if (context.Args.Contains(Execution::Args::Type::InstallerArchitecture))
                {
                    // Arguments provided in command line. Also skips applicability check.
                    allowedArchitectures.emplace_back(Utility::ConvertToArchitectureEnum(context.Args.GetArg(Execution::Args::Type::InstallerArchitecture)));
                    skipApplicabilityCheck = true;
                }
                else
                {
                    auto userIntentItr = metadata.find(Repository::PackageVersionMetadata::UserIntentArchitecture);
                    auto installedItr = metadata.find(Repository::PackageVersionMetadata::InstalledArchitecture);
                    if (userIntentItr != metadata.end())
                    {
                        // For upgrade, user intent from previous install is considered requirement
                        allowedArchitectures.emplace_back(Utility::ConvertToArchitectureEnum(userIntentItr->second));
                    }
                    else
                    {
                        if (installedItr != metadata.end())
                        {
                            // For upgrade, previous installed architecture should be considered first preference and is always allowed.
                            // Then check settings requirements and preferences.
                            allowedArchitectures.emplace_back(Utility::ConvertToArchitectureEnum(installedItr->second));
                        }

                        std::vector<Utility::Architecture> requiredArchitectures = Settings::User().Get<Settings::Setting::InstallArchitectureRequirement>();
                        std::vector<Utility::Architecture> optionalArchitectures = Settings::User().Get<Settings::Setting::InstallArchitecturePreference>();

                        if (!requiredArchitectures.empty())
                        {
                            // Required architecture list from settings if applicable
                            allowedArchitectures.insert(allowedArchitectures.end(), requiredArchitectures.begin(), requiredArchitectures.end());
                        }
                        else
                        {
                            // Preferred architecture list from settings if applicable, add Unknown to indicate allowing remaining applicable
                            if (!optionalArchitectures.empty())
                            {
                                allowedArchitectures.insert(allowedArchitectures.end(), optionalArchitectures.begin(), optionalArchitectures.end());
                            }

                            allowedArchitectures.emplace_back(Utility::Architecture::Unknown);
                        }
                    }
                }

                if (!allowedArchitectures.empty())
                {
                    // If the incoming data contains elements, we will use them to construct a final allowed list.
                    // The algorithm is to take elements until we find Unknown, which indicates that any architecture is
                    // acceptable at this point. The system supported set of architectures will then be placed at the end.
                    std::vector<Utility::Architecture> result;
                    bool addRemainingApplicableArchitectures = false;

                    for (Utility::Architecture architecture : allowedArchitectures)
                    {
                        if (architecture == Utility::Architecture::Unknown)
                        {
                            addRemainingApplicableArchitectures = true;
                            break;
                        }

                        // If the architecture is applicable and not already in our result set...
                        if ((skipApplicabilityCheck || Utility::IsApplicableArchitecture(architecture) != Utility::InapplicableArchitecture) &&
                            Utility::IsApplicableArchitecture(architecture, result) == Utility::InapplicableArchitecture)
                        {
                            result.push_back(architecture);
                        }
                    }

                    if (addRemainingApplicableArchitectures)
                    {
                        for (Utility::Architecture architecture : Utility::GetApplicableArchitectures())
                        {
                            if (Utility::IsApplicableArchitecture(architecture, result) == Utility::InapplicableArchitecture)
                            {
                                result.push_back(architecture);
                            }
                        }
                    }

                    return std::make_unique<MachineArchitectureComparator>(std::move(result));
                }
                else
                {
                    return std::make_unique<MachineArchitectureComparator>();
                }
            }

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                if (CheckAllowedArchitecture(installer.Arch) == Utility::InapplicableArchitecture ||
                    IsSystemArchitectureUnsupportedByInstaller(installer))
                {
                    return InapplicabilityFlags::MachineArchitecture;
                }

                return InapplicabilityFlags::None;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result;
                if (Utility::IsApplicableArchitecture(installer.Arch) == Utility::InapplicableArchitecture)
                {
                    result = "Machine is not compatible with ";
                    result += Utility::ToString(installer.Arch);
                }
                else if (IsSystemArchitectureUnsupportedByInstaller(installer))
                {
                    result = "System architecture is unsupported by installer";
                }
                else
                {
                    result = "Architecture was excluded by caller : ";
                    result += Utility::ToString(installer.Arch);
                }

                return result;
            }

            details::ComparisonResult IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) override
            {
                auto arch1 = CheckAllowedArchitecture(first.Arch);
                auto arch2 = CheckAllowedArchitecture(second.Arch);

                if (arch1 > arch2)
                {
                    // A match with the primary architecture is strong
                    return (first.Arch == GetStrongArchitectureMatch() ? details::ComparisonResult::StrongPositive : details::ComparisonResult::WeakPositive);
                }

                return details::ComparisonResult::Negative;
            }

        private:
            int CheckAllowedArchitecture(Utility::Architecture architecture)
            {
                if (m_allowedArchitectures.empty())
                {
                    return Utility::IsApplicableArchitecture(architecture);
                }
                else
                {
                    return Utility::IsApplicableArchitecture(architecture, m_allowedArchitectures);
                }
            }

            bool IsSystemArchitectureUnsupportedByInstaller(const ManifestInstaller& installer)
            {
                auto unsupportedItr = std::find(
                    installer.UnsupportedOSArchitectures.begin(),
                    installer.UnsupportedOSArchitectures.end(),
                    Utility::GetSystemArchitecture());
                return unsupportedItr != installer.UnsupportedOSArchitectures.end();
            }

            Utility::Architecture GetStrongArchitectureMatch()
            {
                // If we have a preferential order, treat the first entry as strong.
                // Otherwise, treat the system architecture as strong (which is always first in the default order).
                return m_allowedArchitectures.empty() ? Utility::GetSystemArchitecture() : m_allowedArchitectures.front();
            }

            std::vector<Utility::Architecture> m_allowedArchitectures;
        };

        struct InstallerTypeComparator : public details::ComparisonField
        {
            InstallerTypeComparator(std::vector<InstallerTypeEnum> preference, std::vector<InstallerTypeEnum> requirement) :
                details::ComparisonField("Installer Type"), m_preference(std::move(preference)), m_requirement(std::move(requirement))
            {
                m_preferenceAsString = Utility::ConvertContainerToString(m_preference, InstallerTypeToString);
                m_requirementAsString = Utility::ConvertContainerToString(m_requirement, InstallerTypeToString);
                AICLI_LOG(CLI, Verbose,
                    << "InstallerType Comparator created with Required InstallerTypes: " << m_requirementAsString
                    << " , Preferred InstallerTypes: " << m_preferenceAsString);
            }

            static std::unique_ptr<InstallerTypeComparator> Create(const Execution::Args& args)
            {
                std::vector<InstallerTypeEnum> preference;
                std::vector<InstallerTypeEnum> requirement;

                if (args.Contains(Execution::Args::Type::InstallerType))
                {
                    requirement.emplace_back(Manifest::ConvertToInstallerTypeEnum(std::string(args.GetArg(Execution::Args::Type::InstallerType))));
                }
                else
                {
                    preference = Settings::User().Get<Settings::Setting::InstallerTypePreference>();
                    requirement = Settings::User().Get<Settings::Setting::InstallerTypeRequirement>();
                }

                if (!preference.empty() || !requirement.empty())
                {
                    return std::make_unique<InstallerTypeComparator>(preference, requirement);
                }
                else
                {
                    return {};
                }
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "InstallerType [";
                result += InstallerTypeToString(installer.EffectiveInstallerType());
                result += "] does not match required InstallerTypes: ";
                result += m_requirementAsString;
                return result;
            }

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                if (!m_requirement.empty())
                {
                    // The installer is applicable if the effective or base installer type matches.
                    if (ContainsInstallerType(m_requirement, installer.EffectiveInstallerType()) ||
                        ContainsInstallerType(m_requirement, installer.BaseInstallerType))
                    {
                        return InapplicabilityFlags::None;
                    }

                    return InapplicabilityFlags::InstallerType;
                }
                else
                {
                    return InapplicabilityFlags::None;
                }
            }

            details::ComparisonResult IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) override
            {
                if (m_preference.empty())
                {
                    return details::ComparisonResult::Negative;
                }

                for (Manifest::InstallerTypeEnum installerTypePreference : m_preference)
                {
                    bool isFirstInstallerTypePreferred =
                        first.EffectiveInstallerType() == installerTypePreference ||
                        first.BaseInstallerType == installerTypePreference;

                    bool isSecondInstallerTypePreferred =
                        second.EffectiveInstallerType() == installerTypePreference ||
                        second.BaseInstallerType == installerTypePreference;

                    if (isFirstInstallerTypePreferred && isSecondInstallerTypePreferred)
                    {
                        return details::ComparisonResult::Negative;
                    }
                    else if (isFirstInstallerTypePreferred != isSecondInstallerTypePreferred)
                    {
                        // Treating this as a weak positive because one can use requirements to guarantee the installer type if necessary.
                        return (isFirstInstallerTypePreferred ? details::ComparisonResult::WeakPositive : details::ComparisonResult::Negative);
                    }
                }

                return details::ComparisonResult::Negative;
            }

        private:
            std::vector<InstallerTypeEnum> m_preference;
            std::vector<InstallerTypeEnum> m_requirement;
            std::string m_preferenceAsString;
            std::string m_requirementAsString;

            bool ContainsInstallerType(const std::vector<InstallerTypeEnum>& selection, InstallerTypeEnum installerType)
            {
                return std::find(selection.begin(), selection.end(), installerType) != selection.end();
            }
        };

        struct InstalledTypeFilter : public details::FilterField
        {
            InstalledTypeFilter(Manifest::InstallerTypeEnum installedType) :
                details::FilterField("Installed Type"), m_installedType(installedType) {}

            static std::unique_ptr<InstalledTypeFilter> Create(const Repository::IPackageVersion::Metadata& installationMetadata)
            {
                auto installerTypeItr = installationMetadata.find(Repository::PackageVersionMetadata::InstalledType);
                if (installerTypeItr != installationMetadata.end())
                {
                    Manifest::InstallerTypeEnum installedType = Manifest::ConvertToInstallerTypeEnum(installerTypeItr->second);
                    if (installedType != Manifest::InstallerTypeEnum::Unknown)
                    {
                        return std::make_unique<InstalledTypeFilter>(installedType);
                    }
                }

                return {};
            }

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                return IsInstallerCompatibleWith(installer, m_installedType) ? InapplicabilityFlags::None : InapplicabilityFlags::InstalledType;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Installed package type '" + std::string{ Manifest::InstallerTypeToString(m_installedType) } +
                    "' is not compatible with installer type " + std::string{ Manifest::InstallerTypeToString(installer.EffectiveInstallerType()) };

                std::string arpInstallerTypes;
                for (const auto& entry : installer.AppsAndFeaturesEntries)
                {
                    arpInstallerTypes += " " + std::string{ Manifest::InstallerTypeToString(entry.InstallerType) };
                }

                if (!arpInstallerTypes.empty())
                {
                    result += ", or with accepted type(s)" + arpInstallerTypes;
                }

                return result;
            }

        private:
            // The installer is compatible if it's type or any of its ARP entries' type matches the installed type
            static bool IsInstallerCompatibleWith(const Manifest::ManifestInstaller& installer, Manifest::InstallerTypeEnum type)
            {
                if (Manifest::IsInstallerTypeCompatible(installer.EffectiveInstallerType(), type))
                {
                    return true;
                }

                auto itr = std::find_if(
                    installer.AppsAndFeaturesEntries.begin(),
                    installer.AppsAndFeaturesEntries.end(),
                    [=](AppsAndFeaturesEntry arpEntry) { return Manifest::IsInstallerTypeCompatible(arpEntry.InstallerType, type); });
                if (itr != installer.AppsAndFeaturesEntries.end())
                {
                    return true;
                }

                return false;
            }

            Manifest::InstallerTypeEnum m_installedType;
        };

        struct InstalledScopeFilter : public details::FilterField
        {
            InstalledScopeFilter(Manifest::ScopeEnum requirement) :
                details::FilterField("Installed Scope"), m_requirement(requirement) {}

            static std::unique_ptr<InstalledScopeFilter> Create(const Repository::IPackageVersion::Metadata& installationMetadata)
            {
                // Check for an existing install and require a matching scope.
                auto installerScopeItr = installationMetadata.find(Repository::PackageVersionMetadata::InstalledScope);
                if (installerScopeItr != installationMetadata.end())
                {
                    Manifest::ScopeEnum installedScope = Manifest::ConvertToScopeEnum(installerScopeItr->second);
                    if (installedScope != Manifest::ScopeEnum::Unknown)
                    {
                        return std::make_unique<InstalledScopeFilter>(installedScope);
                    }
                }

                return {};
            }

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                // We have to assume the unknown scope will match our required scope, or the entire catalog would stop working for upgrade.
                if (installer.Scope == Manifest::ScopeEnum::Unknown || installer.Scope == m_requirement || DoesInstallerTypeIgnoreScopeFromManifest(installer.EffectiveInstallerType()))
                {
                    return InapplicabilityFlags::None;
                }

                return InapplicabilityFlags::InstalledScope;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Installer scope does not match currently installed scope: ";
                result += Manifest::ScopeToString(installer.Scope);
                result += " != ";
                result += Manifest::ScopeToString(m_requirement);
                return result;
            }

        private:
            Manifest::ScopeEnum m_requirement;
        };

        struct ScopeComparator : public details::ComparisonField
        {
            ScopeComparator(Manifest::ScopeEnum preference, Manifest::ScopeEnum requirement, bool allowUnknownInAdditionToRequired) :
                details::ComparisonField("Scope"), m_preference(preference), m_requirement(requirement), m_allowUnknownInAdditionToRequired(allowUnknownInAdditionToRequired) {}

            static std::unique_ptr<ScopeComparator> Create(const Execution::Context& context)
            {
                // Preference will always come from settings
                Manifest::ScopeEnum preference = Settings::User().Get<Settings::Setting::InstallScopePreference>();

                // Requirement may come from args or settings; args overrides settings.
                Manifest::ScopeEnum requirement = Manifest::ScopeEnum::Unknown;

                const auto& args = context.Args;
                if (args.Contains(Execution::Args::Type::InstallScope))
                {
                    requirement = Manifest::ConvertToScopeEnum(args.GetArg(Execution::Args::Type::InstallScope));
                }
                else
                {
                    requirement = Settings::User().Get<Settings::Setting::InstallScopeRequirement>();
                }

                bool allowUnknownInAdditionToRequired = false;
                if (context.Contains(Execution::Data::AllowUnknownScope))
                {
                    allowUnknownInAdditionToRequired = context.Get<Execution::Data::AllowUnknownScope>();

                    // Force the required type to be preferred over Unknown
                    if (requirement != Manifest::ScopeEnum::Unknown)
                    {
                        preference = requirement;
                    }
                }

                if (preference != Manifest::ScopeEnum::Unknown || requirement != Manifest::ScopeEnum::Unknown)
                {
                    return std::make_unique<ScopeComparator>(preference, requirement, allowUnknownInAdditionToRequired);
                }
                else
                {
                    return {};
                }
            }

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                // Applicable if one of:
                //  1. No requirement (aka is Unknown)
                //  2. Requirement met
                //  3. Installer scope is Unknown and this has been explicitly allowed
                //  4. The installer type is scope agnostic (we can control it)
                if (m_requirement == Manifest::ScopeEnum::Unknown ||
                    installer.Scope == m_requirement ||
                    (installer.Scope == Manifest::ScopeEnum::Unknown && m_allowUnknownInAdditionToRequired) ||
                    DoesInstallerTypeIgnoreScopeFromManifest(installer.EffectiveInstallerType()))
                {
                    return InapplicabilityFlags::None;
                }

                return InapplicabilityFlags::Scope;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Installer scope does not match required scope: ";
                result += Manifest::ScopeToString(installer.Scope);
                result += " != ";
                result += Manifest::ScopeToString(m_requirement);
                return result;
            }

            details::ComparisonResult IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) override
            {
                if (m_preference != Manifest::ScopeEnum::Unknown && first.Scope == m_preference && second.Scope != m_preference)
                {
                    // When the second input is unknown, this is a weak result. If it is not (and therefore the opposite of the preference), this is strong.
                    return (second.Scope == Manifest::ScopeEnum::Unknown ? details::ComparisonResult::WeakPositive : details::ComparisonResult::StrongPositive);
                }

                return details::ComparisonResult::Negative;
            }

        private:
            Manifest::ScopeEnum m_preference;
            Manifest::ScopeEnum m_requirement;
            bool m_allowUnknownInAdditionToRequired;
        };

        struct LocaleComparator : public details::ComparisonField
        {
            LocaleComparator(std::vector<std::string> preference, std::vector<std::string> requirement, bool isInstalledLocale) :
                details::ComparisonField("Locale"), m_preference(std::move(preference)), m_requirement(std::move(requirement)), m_isInstalledLocale(isInstalledLocale)
            {
                m_requirementAsString = Utility::ConvertContainerToString(m_requirement);
                m_preferenceAsString = Utility::ConvertContainerToString(m_preference);
                AICLI_LOG(CLI, Verbose,
                    << "Locale Comparator created with Required Locales: " << m_requirementAsString
                    << " , Preferred Locales: " << m_preferenceAsString
                    << " , IsInstalledLocale: " << m_isInstalledLocale);
            }

            static std::unique_ptr<LocaleComparator> Create(const Execution::Args& args, const Repository::IPackageVersion::Metadata& metadata)
            {
                std::vector<std::string> preference;
                std::vector<std::string> requirement;
                // This is for installed locale case, where the locale is a preference but requires at least compatible match.
                bool isInstalledLocale = false;

                auto userIntentItr = metadata.find(Repository::PackageVersionMetadata::UserIntentLocale);
                auto installedItr = metadata.find(Repository::PackageVersionMetadata::InstalledLocale);
                // Requirement may come from args, previous user intent or settings; args overrides previous user intent then settings.
                if (args.Contains(Execution::Args::Type::Locale))
                {
                    requirement.emplace_back(args.GetArg(Execution::Args::Type::Locale));
                }
                else if (userIntentItr != metadata.end())
                {
                    requirement.emplace_back(userIntentItr->second);
                    isInstalledLocale = true;
                }
                else
                {
                    if (installedItr == metadata.end())
                    {
                        // If it's an upgrade of previous package, no need to set requirements from settings
                        // as previous installed locale will be used later.
                        requirement = Settings::User().Get<Settings::Setting::InstallLocaleRequirement>();
                    }
                }

                // Preference will come from previous installed locale, winget settings or Preferred Languages settings.
                // Previous installed locale goes first, then winget settings, then Preferred Languages settings.
                // Previous installed locale also requires at least compatible locale match.
                if (installedItr != metadata.end())
                {
                    preference.emplace_back(installedItr->second);
                    isInstalledLocale = true;
                }
                else
                {
                    preference = Settings::User().Get<Settings::Setting::InstallLocalePreference>();
                    if (preference.empty())
                    {
                        preference = Locale::GetUserPreferredLanguages();
                    }
                }

                if (!preference.empty() || !requirement.empty())
                {
                    return std::make_unique<LocaleComparator>(preference, requirement, isInstalledLocale);
                }
                else
                {
                    return {};
                }
            }

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                InapplicabilityFlags inapplicableFlag = m_isInstalledLocale ? InapplicabilityFlags::InstalledLocale : InapplicabilityFlags::Locale;

                if (!m_requirement.empty())
                {
                    // Check if requirement is satisfied
                    for (auto const& requiredLocale : m_requirement)
                    {
                        if (Locale::GetDistanceOfLanguage(requiredLocale, installer.Locale) >= Locale::MinimumDistanceScoreAsPerfectMatch)
                        {
                            return InapplicabilityFlags::None;
                        }
                    }

                    return inapplicableFlag;
                }
                else if (m_isInstalledLocale && !m_preference.empty())
                {
                    // For installed locale preference, check at least compatible match for preference
                    for (auto const& preferredLocale : m_preference)
                    {
                        // We have to assume an unknown installer locale will match our installed locale, or the entire catalog would stop working for upgrade.
                        if (installer.Locale.empty() ||
                            Locale::GetDistanceOfLanguage(preferredLocale, installer.Locale) >= Locale::MinimumDistanceScoreAsCompatibleMatch)
                        {
                            return InapplicabilityFlags::None;
                        }
                    }

                    return inapplicableFlag;
                }
                else
                {
                    return InapplicabilityFlags::None;
                }
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Installer locale does not match required locale: ";
                result += installer.Locale;
                result += "Required locales: ";
                result += m_requirementAsString;
                result += " Or does not satisfy compatible match for Preferred Locales: ";
                result += m_preferenceAsString;
                return result;
            }

            details::ComparisonResult IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) override
            {
                if (m_preference.empty())
                {
                    return details::ComparisonResult::Negative;
                }

                for (auto const& preferredLocale : m_preference)
                {
                    double firstScore = first.Locale.empty() ? Locale::UnknownLanguageDistanceScore : Locale::GetDistanceOfLanguage(preferredLocale, first.Locale);
                    double secondScore = second.Locale.empty() ? Locale::UnknownLanguageDistanceScore : Locale::GetDistanceOfLanguage(preferredLocale, second.Locale);

                    if (firstScore >= Locale::MinimumDistanceScoreAsCompatibleMatch || secondScore >= Locale::MinimumDistanceScoreAsCompatibleMatch)
                    {
                        // This could probably be enriched to always check all locales and determine strong/weak based off of the MinimumDistanceScoreAsCompatibleMatch.
                        return (firstScore > secondScore ? details::ComparisonResult::StrongPositive : details::ComparisonResult::Negative);
                    }
                }

                // At this point, the installer locale matches no preference.
                // if first is unknown and second is no match for sure, we might prefer unknown one.
                return (first.Locale.empty() && !second.Locale.empty() ? details::ComparisonResult::WeakPositive : details::ComparisonResult::Negative);
            }

        private:
            std::vector<std::string> m_preference;
            std::vector<std::string> m_requirement;
            std::string m_requirementAsString;
            std::string m_preferenceAsString;
            bool m_isInstalledLocale = false;
        };

        struct MarketFilter : public details::FilterField
        {
            MarketFilter(Manifest::string_t market) : details::FilterField("Market"), m_market(market)
            {
                AICLI_LOG(CLI, Verbose, << "Market Filter created with market: " << m_market);
            }

            static std::unique_ptr<MarketFilter> Create()
            {
                return std::make_unique<MarketFilter>(Runtime::GetOSRegion());
            }

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                // If both allowed and excluded lists are provided, we only need to check the allowed markets.
                if (!installer.Markets.AllowedMarkets.empty())
                {
                    // Inapplicable if NOT found
                    if (!IsMarketInList(installer.Markets.AllowedMarkets))
                    {
                        return InapplicabilityFlags::Market;
                    }
                }
                else if (!installer.Markets.ExcludedMarkets.empty())
                {
                    // Inapplicable if found
                    if (IsMarketInList(installer.Markets.ExcludedMarkets))
                    {
                        return InapplicabilityFlags::Market;
                    }
                }

                return InapplicabilityFlags::None;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Current market '" + m_market + "' does not match installer markets." +
                    " Allowed markets: " + Utility::ConvertContainerToString(installer.Markets.AllowedMarkets) +
                    " Excluded markets: " + Utility::ConvertContainerToString(installer.Markets.ExcludedMarkets);
                return result;
            }

        private:
            bool IsMarketInList(const std::vector<Manifest::string_t> markets)
            {
                return markets.end() != std::find_if(
                    markets.begin(),
                    markets.end(),
                    [&](const auto& m) { return Utility::CaseInsensitiveEquals(m, m_market); });
            }

            Manifest::string_t m_market;
        };
    }

    ManifestComparator::ManifestComparator(const Execution::Context& context, const Repository::IPackageVersion::Metadata& installationMetadata)
    {
        // Filters based on installer's MinOSVersion
        AddFilter(std::make_unique<OSVersionFilter>());
        // Filters out portable installers if they are not supported by the system
        AddFilter(std::make_unique<PortableInstallFilter>());
        // Filters based on the scope of a currently installed package
        AddFilter(InstalledScopeFilter::Create(installationMetadata));
        // Filters based on the market region of the system
        AddFilter(MarketFilter::Create());
        // Filters based on the installer type compatability, including with AppsAndFeaturesEntry declarations
        AddFilter(InstalledTypeFilter::Create(installationMetadata));

        // Filter order is not important, but comparison order determines priority.
        // Note that all comparators are also filters and their comparison function will only be called on
        // installers that both match the required criteria.
        // 
        // The comparators are ordered by the `IsFirstBetter` method, which uses the following algorithm:
        //  - Each comparison between two installers can return one of { Strong, Weak, Negative }
        //  - Installers are compared in both directions, going through the list of comparators as defined here
        //  - The first Strong result in either direction is given priority
        //  - If no Strong results, the first Weak result is used
        //  - If all Negative results, then the two installers are equal in priority (meaning the first one in the list is kept as "better")
        // 
        // TODO: There are improvements to be made here around ordering, especially in the context of implicit vs explicit vs command line preferences.

        // Filters based on exact matches for requirements or compatible matches for preferences
        // Only applies when preference exists:
        // Strong if first is compatible and better match than second
        // Weak if first is unknown and second is not
        AddComparator(LocaleComparator::Create(context.Args, installationMetadata));
        // Filters only if a requirement is present and it cannot be satisfied by the installer (including installer types that we can control scope in code)
        // Only applies when preference exists:
        // Strong if first matches preference and second does not and is not Unknown
        // Weak if first matches preference and second is Unknown
        AddComparator(ScopeComparator::Create(context));
        // Filters architectures out that are not supported or are not in the preferences/requirements/inputs.
        // Strong if first equals the earliest architecture in the allowed list and second does not [default means the system architecture]
        // Weak if first is better match for system architecture than second
        AddComparator(MachineArchitectureComparator::Create(context, installationMetadata));
        // Filters installer types out that are not in preferences or requirements.
        // Only applies when preference exists:
        // Weak if first is in preference list and second is not
        AddComparator(InstallerTypeComparator::Create(context.Args));
    }

    InstallerAndInapplicabilities ManifestComparator::GetPreferredInstaller(const Manifest::Manifest& manifest)
    {
        AICLI_LOG(CLI, Verbose, << "Starting installer selection.");

        const Manifest::ManifestInstaller* result = nullptr;
        std::vector<InapplicabilityFlags> inapplicabilitiesInstallers;

        for (const auto& installer : manifest.Installers)
        {
            auto inapplicabilityInstaller = IsApplicable(installer);
            if (inapplicabilityInstaller == InapplicabilityFlags::None)
            {
                if (!result || IsFirstBetter(installer, *result))
                {
                    AICLI_LOG(CLI, Verbose, << "Installer " << installer << " is current best choice");
                    result = &installer;
                }
            }
            else
            {
                inapplicabilitiesInstallers.push_back(inapplicabilityInstaller);
            }
        }

        if (!result)
        {
            return { {}, std::move(inapplicabilitiesInstallers) };
        }

        return { *result, std::move(inapplicabilitiesInstallers) };
    }

    InapplicabilityFlags ManifestComparator::IsApplicable(const Manifest::ManifestInstaller& installer)
    {
        InapplicabilityFlags inapplicabilityResult = InapplicabilityFlags::None;

        for (const auto& filter : m_filters)
        {
            auto inapplicability = filter->IsApplicable(installer);
            if (inapplicability != InapplicabilityFlags::None)
            {
                AICLI_LOG(CLI, Verbose, << "Installer " << installer << " not applicable: " << filter->ExplainInapplicable(installer));
                WI_SetAllFlags(inapplicabilityResult, inapplicability);
            }
        }

        return inapplicabilityResult;
    }

    bool ManifestComparator::IsFirstBetter(
        const Manifest::ManifestInstaller& first,
        const Manifest::ManifestInstaller& second)
    {
        // The priority will still be used as a tie-break between weak results.
        std::optional<std::string_view> firstWeakComparator;
        bool firstWeakComparatorResult = false;

        for (auto comparator : m_comparators)
        {
            details::ComparisonResult forwardCompare = comparator->IsFirstBetter(first, second);
            details::ComparisonResult reverseCompare = comparator->IsFirstBetter(second, first);

            // Should not happen, but if it does it points at a serious bug that should be fixed.
            if (forwardCompare != details::ComparisonResult::Negative && reverseCompare != details::ComparisonResult::Negative)
            {
                AICLI_LOG(CLI, Error, << "Installer " << first << " and " << second << " are both better than each other?");
                THROW_HR(E_UNEXPECTED);
            }

            if (forwardCompare == details::ComparisonResult::StrongPositive)
            {
                AICLI_LOG(CLI, Verbose, << "Installer " << first << " is better [strong] than " << second << " due to: " << comparator->Name());
                return true;
            }

            if (reverseCompare == details::ComparisonResult::StrongPositive)
            {
                // Second is better by this comparator, don't allow a lower priority one to override that.
                AICLI_LOG(CLI, Verbose, << "Installer " << second << " is better [strong] than " << first << " due to: " << comparator->Name());
                return false;
            }

            // Save the first weak result that we get
            if (!firstWeakComparator)
            {
                if (forwardCompare == details::ComparisonResult::WeakPositive)
                {
                    firstWeakComparator = comparator->Name();
                    firstWeakComparatorResult = true;
                }
                else if (reverseCompare == details::ComparisonResult::WeakPositive)
                {
                    firstWeakComparator = comparator->Name();
                    firstWeakComparatorResult = false;
                }
            }
        }

        // If we found a weak result (and no strong result because we made it here), return it.
        if (firstWeakComparator)
        {
            if (firstWeakComparatorResult)
            {
                AICLI_LOG(CLI, Verbose, << "Installer " << first << " is better [weak] than " << second << " due to: " << *firstWeakComparator);
            }
            else
            {
                AICLI_LOG(CLI, Verbose, << "Installer " << second << " is better [weak] than " << first << " due to: " << *firstWeakComparator);
            }

            return firstWeakComparatorResult;
        }

        // Equal, and thus not better
        AICLI_LOG(CLI, Verbose, << "Installer " << first << " and " << second << " are equivalent in priority");
        return false;
    }

    void ManifestComparator::AddFilter(std::unique_ptr<details::FilterField>&& filter)
    {
        if (filter)
        {
            m_filters.emplace_back(std::move(filter));
        }
    }

    void ManifestComparator::AddComparator(std::unique_ptr<details::ComparisonField>&& comparator)
    {
        if (comparator)
        {
            m_comparators.push_back(comparator.get());
            m_filters.emplace_back(std::move(comparator));
        }
    }
}
