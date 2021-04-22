// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowBase.h"
#include "ManifestComparator.h"
#include <winget/UserSettings.h>

using namespace AppInstaller::CLI;
using namespace AppInstaller::Manifest;

std::ostream& operator<<(std::ostream& out, const AppInstaller::Manifest::ManifestInstaller& installer)
{
    return out << '[' <<
        AppInstaller::Utility::ToString(installer.Arch) << ',' <<
        AppInstaller::Manifest::InstallerTypeToString(installer.InstallerType) << ',' <<
        AppInstaller::Manifest::ScopeToString(installer.Scope) << ',' <<
        installer.Locale << ']';
}

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        struct OSVersionFilter : public details::FilterField
        {
            OSVersionFilter() : details::FilterField("OS Version") {}

            bool IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                return installer.MinOSVersion.empty() || Runtime::IsCurrentOSVersionGreaterThanOrEqual(Utility::Version(installer.MinOSVersion));
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

            bool IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                return Utility::IsApplicableArchitecture(installer.Arch) != Utility::InapplicableArchitecture;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Machine is not compatible with ";
                result += Utility::ToString(installer.Arch);
                return result;
            }

            bool IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) override
            {
                auto arch1 = Utility::IsApplicableArchitecture(first.Arch);
                auto arch2 = Utility::IsApplicableArchitecture(second.Arch);

                if (arch1 > arch2)
                {
                    return true;
                }

                return false;
            }
        };

        struct InstalledTypeComparator : public details::ComparisonField
        {
            InstalledTypeComparator(Manifest::InstallerTypeEnum installedType) :
                details::ComparisonField("Installed Type"), m_installedType(installedType) {}

            static std::unique_ptr<InstalledTypeComparator> Create(const Repository::IPackageVersion::Metadata& installationMetadata)
            {
                auto installerTypeItr = installationMetadata.find(Repository::PackageVersionMetadata::InstalledType);
                if (installerTypeItr != installationMetadata.end())
                {
                    Manifest::InstallerTypeEnum installedType = Manifest::ConvertToInstallerTypeEnum(installerTypeItr->second);
                    if (installedType != Manifest::InstallerTypeEnum::Unknown)
                    {
                        return std::make_unique<InstalledTypeComparator>(installedType);
                    }
                }

                return {};
            }

            bool IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                return Manifest::IsInstallerTypeCompatible(installer.InstallerType, m_installedType);
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Installed package type is not compatible with ";
                result += Manifest::InstallerTypeToString(installer.InstallerType);
                return result;
            }

            bool IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) override
            {
                return (first.InstallerType == m_installedType && second.InstallerType != m_installedType);
            }

        private:
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

            bool IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                // We have to assume the unknown scope will match our required scope, or the entire catalog would stop working for upgrade.
                return installer.Scope == Manifest::ScopeEnum::Unknown || installer.Scope == m_requirement;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Installer scope does not matched currently installed scope: ";
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
            ScopeComparator(Manifest::ScopeEnum preference, Manifest::ScopeEnum requirement) :
                details::ComparisonField("Scope"), m_preference(preference), m_requirement(requirement) {}

            static std::unique_ptr<ScopeComparator> Create(const Execution::Args& args)
            {
                // Preference will always come from settings
                Manifest::ScopeEnum preference = ConvertScope(Settings::User().Get<Settings::Setting::InstallScopePreference>());

                // Requirement may come from args or settings; args overrides settings.
                Manifest::ScopeEnum requirement = Manifest::ScopeEnum::Unknown;

                if (args.Contains(Execution::Args::Type::InstallScope))
                {
                    requirement = Manifest::ConvertToScopeEnum(args.GetArg(Execution::Args::Type::InstallScope));
                }
                else
                {
                    requirement = ConvertScope(Settings::User().Get<Settings::Setting::InstallScopeRequirement>());
                }

                if (preference != Manifest::ScopeEnum::Unknown || requirement != Manifest::ScopeEnum::Unknown)
                {
                    return std::make_unique<ScopeComparator>(preference, requirement);
                }
                else
                {
                    return {};
                }
            }

            bool IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                return m_requirement == Manifest::ScopeEnum::Unknown || installer.Scope == m_requirement;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Installer scope does not match required scope: ";
                result += Manifest::ScopeToString(installer.Scope);
                result += " != ";
                result += Manifest::ScopeToString(m_requirement);
                return result;
            }

            bool IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) override
            {
                return m_preference != Manifest::ScopeEnum::Unknown && (first.Scope == m_preference && second.Scope != m_preference);
            }

        private:
            static Manifest::ScopeEnum ConvertScope(Settings::ScopePreference scope)
            {
                switch (scope)
                {
                case Settings::ScopePreference::None: return Manifest::ScopeEnum::Unknown;
                case Settings::ScopePreference::User: return Manifest::ScopeEnum::User;
                case Settings::ScopePreference::Machine: return Manifest::ScopeEnum::Machine;
                }

                return Manifest::ScopeEnum::Unknown;
            }

            Manifest::ScopeEnum m_preference;
            Manifest::ScopeEnum m_requirement;
        };

        struct InstalledLocaleFilter : public details::FilterField
        {
            InstalledLocaleFilter(std::string requirement) :
                details::FilterField("Installed Locale"), m_requirement(std::move(requirement)) {}

            static std::unique_ptr<InstalledLocaleFilter> Create(const Repository::IPackageVersion::Metadata& installationMetadata)
            {
                // Check for an existing install and require a matching scope.
                auto installerLocaleItr = installationMetadata.find(Repository::PackageVersionMetadata::InstalledLocale);
                if (installerLocaleItr != installationMetadata.end())
                {
                    return std::make_unique<InstalledLocaleFilter>(installerLocaleItr->second);
                }

                return {};
            }

            bool IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                // We have to assume an unknown installer locale will match our required locale, or the entire catalog would stop working for upgrade.
                return installer.Locale.empty() || Utility::GetDistanceOfLanguage(m_requirement, installer.Locale) >= Utility::MinimumDistanceScoreAsPerfectMatch;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Installer locale does not matched currently installed locale: ";
                result += installer.Locale;
                result += " != ";
                result += m_requirement;
                return result;
            }

        private:
            std::string m_requirement;
        };

        struct LocaleComparator : public details::ComparisonField
        {
            LocaleComparator(std::string preference, std::string requirement) :
                details::ComparisonField("Locale"), m_preference(std::move(preference)), m_requirement(std::move(requirement)) {}

            static std::unique_ptr<LocaleComparator> Create(const Repository::IPackageVersion::Metadata& installationMetadata, const Execution::Args& args)
            {
                std::string preference;
                std::string requirement;

                auto installerLocaleItr = installationMetadata.find(Repository::PackageVersionMetadata::InstalledLocale);
                if (installerLocaleItr != installationMetadata.end())
                {
                    // Locale upgrade applicability based on installed locale should already be done by InstalledLocaleFilter.
                    // For now we only allow perfect match or unknown locale. If we are to relax the rule a bit,
                    // then for upgrade with known installed locale, use the installed locale as preference.
                    preference = installerLocaleItr->second;
                }
                else
                {
                    // Preference will come from winget settings or Preferred Languages settings. winget settings takes precedence.
                    preference = Settings::User().Get<Settings::Setting::InstallLocalePreference>();
                    if (preference.empty())
                    {
                        auto preferredList = Utility::GetUserPreferredLanguages();
                        if (!preferredList.empty())
                        {
                            // TODO: we only take the first one for now
                            preference = preferredList.at(0);
                        }
                    }

                    // Requirement may come from args or settings; args overrides settings.
                    if (args.Contains(Execution::Args::Type::Locale))
                    {
                        requirement = args.GetArg(Execution::Args::Type::Locale);
                    }
                    else
                    {
                        requirement = Settings::User().Get<Settings::Setting::InstallLocaleRequirement>();
                    }
                }

                if (!preference.empty() || !requirement.empty())
                {
                    return std::make_unique<LocaleComparator>(preference, requirement);
                }
                else
                {
                    return {};
                }
            }

            bool IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                return m_requirement.empty() || Utility::GetDistanceOfLanguage(m_requirement, installer.Locale) >= Utility::MinimumDistanceScoreAsPerfectMatch;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Installer locale does not match required locale: ";
                result += installer.Locale;
                result += " != ";
                result += m_requirement;
                return result;
            }

            bool IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) override
            {
                if (m_preference.empty())
                {
                    return false;
                }

                double firstScore = first.Locale.empty() ? Utility::UnknownLanguageDistanceScore : Utility::GetDistanceOfLanguage(m_preference, first.Locale);
                double secondScore = second.Locale.empty() ? Utility::UnknownLanguageDistanceScore : Utility::GetDistanceOfLanguage(m_preference, second.Locale);

                if (firstScore > secondScore)
                {
                    return true;
                }
                else if (firstScore == secondScore && firstScore == Utility::UnknownLanguageDistanceScore)
                {
                    // if first is unknown and second is no match for sure, we might prefer unknown one.
                    return first.Locale.empty() && !second.Locale.empty();
                }

                return false;
            }

        private:
            std::string m_preference;
            std::string m_requirement;
        };
    }

    ManifestComparator::ManifestComparator(const Execution::Args& args, const Repository::IPackageVersion::Metadata& installationMetadata)
    {
        AddFilter(std::make_unique<OSVersionFilter>());
        AddFilter(InstalledScopeFilter::Create(installationMetadata));
        AddFilter(InstalledLocaleFilter::Create(installationMetadata));

        // Filter order is not important, but comparison order determines priority.
        // TODO: There are improvements to be made here around ordering, especially in the context of implicit vs explicit vs command line preferences.
        AddComparator(InstalledTypeComparator::Create(installationMetadata));
        AddComparator(ScopeComparator::Create(args));
        AddComparator(LocaleComparator::Create(installationMetadata, args));
        AddComparator(std::make_unique<MachineArchitectureComparator>());
    }

    std::optional<Manifest::ManifestInstaller> ManifestComparator::GetPreferredInstaller(const Manifest::Manifest& manifest)
    {
        AICLI_LOG(CLI, Info, << "Starting installer selection.");

        const Manifest::ManifestInstaller* result = nullptr;

        for (const auto& installer : manifest.Installers)
        {
            if (IsApplicable(installer) && (!result || IsFirstBetter(installer, *result)))
            {
                AICLI_LOG(CLI, Verbose, << "Installer " << installer << " is current best choice");
                result = &installer;
            }
        }

        if (!result)
        {
            return {};
        }

        Logging::Telemetry().LogSelectedInstaller(
            static_cast<int>(result->Arch),
            result->Url,
            Manifest::InstallerTypeToString(result->InstallerType),
            Manifest::ScopeToString(result->Scope),
            result->Locale);

        return *result;
    }

    // TODO: Implement a mechanism for better error messaging for no applicable installer scenario
    bool ManifestComparator::IsApplicable(const Manifest::ManifestInstaller& installer)
    {
        for (const auto& filter : m_filters)
        {
            if (!filter->IsApplicable(installer))
            {
                AICLI_LOG(CLI, Info, << "Installer " << installer << " not applicable: " << filter->ExplainInapplicable(installer));
                return false;
            }
        }

        return true;
    }

    bool ManifestComparator::IsFirstBetter(
        const Manifest::ManifestInstaller& first,
        const Manifest::ManifestInstaller& second)
    {
        for (auto comparator : m_comparators)
        {
            if (comparator->IsFirstBetter(first, second))
            {
                AICLI_LOG(CLI, Verbose, << "Installer " << first << " is better than " << second << " due to: " << comparator->Name());
                return true;
            }
            else if (comparator->IsFirstBetter(second, first))
            {
                // Second is better by this comparator, don't allow a lower priority one to override that.
                AICLI_LOG(CLI, Verbose, << "Installer " << second << " is better than " << first << " due to: " << comparator->Name());
                return false;
            }
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