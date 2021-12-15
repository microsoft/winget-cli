// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowBase.h"
#include "ExecutionContext.h"
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
                AICLI_LOG(CLI, Verbose, << "Architecture Comparator created with allowed architectures: " << GetAllowedArchitecturesString());
            }

            // TODO: At some point we can do better about matching the currently installed architecture
            static std::unique_ptr<MachineArchitectureComparator> Create(const Execution::Context& context, const Repository::IPackageVersion::Metadata&)
            {
                if (context.Contains(Execution::Data::AllowedArchitectures))
                {
                    const std::vector<Utility::Architecture>& allowedArchitectures = context.Get<Execution::Data::AllowedArchitectures>();
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
                            if (Utility::IsApplicableArchitecture(architecture) != Utility::InapplicableArchitecture &&
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
                }

                return std::make_unique<MachineArchitectureComparator>();
            }

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                if (CheckAllowedArchitecture(installer.Arch) == Utility::InapplicableArchitecture)
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
                }
                else
                {
                    result = "Architecture was excluded by caller : ";
                }
                result += Utility::ToString(installer.Arch);
                return result;
            }

            bool IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) override
            {
                auto arch1 = CheckAllowedArchitecture(first.Arch);
                auto arch2 = CheckAllowedArchitecture(second.Arch);

                if (arch1 > arch2)
                {
                    return true;
                }

                return false;
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

            std::string GetAllowedArchitecturesString()
            {
                std::stringstream result;
                bool prependComma = false;
                for (Utility::Architecture architecture : m_allowedArchitectures)
                {
                    if (prependComma)
                    {
                        result << ", ";
                    }
                    result << Utility::ToString(architecture);
                    prependComma = true;
                }
                return result.str();
            }

            std::vector<Utility::Architecture> m_allowedArchitectures;
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

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                if (Manifest::IsInstallerTypeCompatible(installer.InstallerType, m_installedType))
                {
                    return InapplicabilityFlags::None;
                }

                return InapplicabilityFlags::InstalledType;
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

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                // We have to assume the unknown scope will match our required scope, or the entire catalog would stop working for upgrade.
                if (installer.Scope == Manifest::ScopeEnum::Unknown || installer.Scope == m_requirement)
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

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                if (m_requirement == Manifest::ScopeEnum::Unknown || installer.Scope == m_requirement)
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

        struct InstalledLocaleComparator : public details::ComparisonField
        {
            InstalledLocaleComparator(std::string installedLocale) :
                details::ComparisonField("Installed Locale"), m_installedLocale(std::move(installedLocale)) {}

            static std::unique_ptr<InstalledLocaleComparator> Create(const Repository::IPackageVersion::Metadata& installationMetadata)
            {
                // Check for an existing install and require a compatible locale.
                auto installerLocaleItr = installationMetadata.find(Repository::PackageVersionMetadata::InstalledLocale);
                if (installerLocaleItr != installationMetadata.end())
                {
                    return std::make_unique<InstalledLocaleComparator>(installerLocaleItr->second);
                }

                return {};
            }

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                // We have to assume an unknown installer locale will match our installed locale, or the entire catalog would stop working for upgrade.
                if (installer.Locale.empty() ||
                    Locale::GetDistanceOfLanguage(m_installedLocale, installer.Locale) >= Locale::MinimumDistanceScoreAsCompatibleMatch)
                {
                    return InapplicabilityFlags::None;
                }

                return InapplicabilityFlags::InstalledLocale;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Installer locale is not compatible with currently installed locale: ";
                result += installer.Locale;
                result += " not compatible with ";
                result += m_installedLocale;
                return result;
            }

            bool IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) override
            {
                double firstScore = first.Locale.empty() ? Locale::UnknownLanguageDistanceScore : Locale::GetDistanceOfLanguage(m_installedLocale, first.Locale);
                double secondScore = second.Locale.empty() ? Locale::UnknownLanguageDistanceScore : Locale::GetDistanceOfLanguage(m_installedLocale, second.Locale);

                return firstScore > secondScore;
            }

        private:
            std::string m_installedLocale;
        };

        struct LocaleComparator : public details::ComparisonField
        {
            LocaleComparator(std::vector<std::string> preference, std::vector<std::string> requirement) :
                details::ComparisonField("Locale"), m_preference(std::move(preference)), m_requirement(std::move(requirement))
            {
                m_requirementAsString = GetLocalesListAsString(m_requirement);
                m_preferenceAsString = GetLocalesListAsString(m_preference);
                AICLI_LOG(CLI, Verbose, << "Locale Comparator created with Required Locales: " << m_requirementAsString << " , Preferred Locales: " << m_preferenceAsString);
            }

            static std::unique_ptr<LocaleComparator> Create(const Execution::Args& args)
            {
                std::vector<std::string> preference;
                std::vector<std::string> requirement;

                // Preference will come from winget settings or Preferred Languages settings. winget settings takes precedence.
                preference = Settings::User().Get<Settings::Setting::InstallLocalePreference>();
                if (preference.empty())
                {
                    preference = Locale::GetUserPreferredLanguages();
                }

                // Requirement may come from args or settings; args overrides settings.
                if (args.Contains(Execution::Args::Type::Locale))
                {
                    requirement.emplace_back(args.GetArg(Execution::Args::Type::Locale));
                }
                else
                {
                    requirement = Settings::User().Get<Settings::Setting::InstallLocaleRequirement>();
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

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                if (m_requirement.empty())
                {
                    return InapplicabilityFlags::None;
                }

                for (auto const& requiredLocale : m_requirement)
                {
                    if (Locale::GetDistanceOfLanguage(requiredLocale, installer.Locale) >= Locale::MinimumDistanceScoreAsPerfectMatch)
                    {
                        return InapplicabilityFlags::None;
                    }
                }

                return InapplicabilityFlags::Locale;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Installer locale does not match required locale: ";
                result += installer.Locale;
                result += "Required locales: ";
                result += m_requirementAsString;
                return result;
            }

            bool IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) override
            {
                if (m_preference.empty())
                {
                    return false;
                }

                for (auto const& preferredLocale : m_preference)
                {
                    double firstScore = first.Locale.empty() ? Locale::UnknownLanguageDistanceScore : Locale::GetDistanceOfLanguage(preferredLocale, first.Locale);
                    double secondScore = second.Locale.empty() ? Locale::UnknownLanguageDistanceScore : Locale::GetDistanceOfLanguage(preferredLocale, second.Locale);

                    if (firstScore >= Locale::MinimumDistanceScoreAsCompatibleMatch || secondScore >= Locale::MinimumDistanceScoreAsCompatibleMatch)
                    {
                        return firstScore > secondScore;
                    }
                }

                // At this point, the installer locale matches no preference.
                // if first is unknown and second is no match for sure, we might prefer unknown one.
                return first.Locale.empty() && !second.Locale.empty();
            }

        private:
            std::vector<std::string> m_preference;
            std::vector<std::string> m_requirement;
            std::string m_requirementAsString;
            std::string m_preferenceAsString;

            std::string GetLocalesListAsString(const std::vector<std::string>& locales)
            {
                std::string result = "[";

                bool first = true;
                for (auto const& locale : locales)
                {
                    if (first)
                    {
                        first = false;
                    }
                    else
                    {
                        result += ", ";
                    }

                    result += locale;
                }

                result += ']';

                return result;
            }
        };

        struct MarketFilter : public details::FilterField
        {
            MarketFilter(Manifest::string_t market) : details::FilterField("Market"), m_market(market)
            {
                AICLI_LOG(CLI, Verbose, << "Market Comparator created with market: " << m_market);
            }

            static std::unique_ptr<MarketFilter> Create()
            {
                return std::make_unique<MarketFilter>(GetCurrentMarket());
            }

            InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) override
            {
                // If both allowed and excluded lists are provided, we only need to check the allowed markets.
                if (!installer.Markets.AllowedMarkets.empty())
                {
                    // Inapplicable if NOT found
                    if (installer.Markets.AllowedMarkets.end() == std::find(installer.Markets.AllowedMarkets.begin(), installer.Markets.AllowedMarkets.end(), m_market))
                    {
                        return InapplicabilityFlags::Market;
                    }
                }
                else if (!installer.Markets.ExcludedMarkets.empty())
                {
                    // Inapplicable if found
                    if (installer.Markets.ExcludedMarkets.end() != std::find(installer.Markets.ExcludedMarkets.begin(), installer.Markets.ExcludedMarkets.end(), m_market))
                    {
                        return InapplicabilityFlags::Market;
                    }
                }

                return InapplicabilityFlags::None;
            }

            std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) override
            {
                std::string result = "Current market '" + m_market + "' does not match installer markets." +
                    " Allowed markets: " + GetMarketsListAsString(installer.Markets.AllowedMarkets) +
                    " Excluded markets: " + GetMarketsListAsString(installer.Markets.ExcludedMarkets);
                return result;
            }

        private:
            Manifest::string_t m_market;

            static Manifest::string_t GetCurrentMarket()
            {
                // TODO: Manifest uses 2-letter codes (ISO 3166?) but GetUserDefaultGeoName can return
                // either a 2-letter ISO-3166 code or a numeric UN M.49 code.
                auto geoNameSize = GetUserDefaultGeoName(nullptr, 0);
                THROW_LAST_ERROR_IF(geoNameSize == 0);

                std::vector<wchar_t> geoName(geoNameSize);
                geoNameSize = GetUserDefaultGeoName(geoName.data(), geoNameSize);
                THROW_LAST_ERROR_IF(geoNameSize == 0);

                return Manifest::string_t{ geoName.data() };
            }

            std::string GetMarketsListAsString(const std::vector<Manifest::string_t>& markets)
            {
                // TODO: Refactor to merge with GetLocalesListAsString and GetAllowedArchitecturesString
                std::string result = "[";

                bool first = true;
                for (auto const& market : markets)
                {
                    if (first)
                    {
                        first = false;
                    }
                    else
                    {
                        result += ", ";
                    }

                    result += market;
                }

                result += ']';

                return result;
            }
        };
    }

    ManifestComparator::ManifestComparator(const Execution::Context& context, const Repository::IPackageVersion::Metadata& installationMetadata)
    {
        AddFilter(std::make_unique<OSVersionFilter>());
        AddFilter(InstalledScopeFilter::Create(installationMetadata));
        AddFilter(MarketFilter::Create());

        // Filter order is not important, but comparison order determines priority.
        // TODO: There are improvements to be made here around ordering, especially in the context of implicit vs explicit vs command line preferences.
        AddComparator(InstalledTypeComparator::Create(installationMetadata));

        auto installedLocaleComparator = InstalledLocaleComparator::Create(installationMetadata);
        if (installedLocaleComparator)
        {
            AddComparator(std::move(installedLocaleComparator));
        }
        else
        {
            AddComparator(LocaleComparator::Create(context.Args));
        }

        AddComparator(ScopeComparator::Create(context.Args));
        AddComparator(MachineArchitectureComparator::Create(context, installationMetadata));
    }

    InstallerAndInapplicabilities ManifestComparator::GetPreferredInstaller(const Manifest::Manifest& manifest)
    {
        AICLI_LOG(CLI, Info, << "Starting installer selection.");

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
                AICLI_LOG(CLI, Info, << "Installer " << installer << " not applicable: " << filter->ExplainInapplicable(installer));
                WI_SetAllFlags(inapplicabilityResult, inapplicability);
            }
        }

        return inapplicabilityResult;
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