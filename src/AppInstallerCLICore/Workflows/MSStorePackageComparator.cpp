// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MSStorePackageComparator.h"
#include "WorkflowBase.h"
#include <AppInstallerLogging.h>
#include <winget/UserSettings.h>
#include <winget/MSStoreDownload.h>
#include <winget/Locale.h>

using namespace AppInstaller::CLI;
using namespace AppInstaller::Manifest;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        bool IsPackageFormatBundle(MSStore::PackageFormatEnum packageFormatEnum)
        {
            return
                packageFormatEnum == MSStore::PackageFormatEnum::AppxBundle ||
                packageFormatEnum == MSStore::PackageFormatEnum::MsixBundle;
        }

        struct PackageFormatFilter : public details::MSStorePackageComparisonField
        {
            PackageFormatFilter() : details::MSStorePackageComparisonField("Package Format") {}

            bool IsApplicable(const MSStore::MSStoreDisplayCatalogPackage& package) override
            {
                if (package.PackageFormat == MSStore::PackageFormatEnum::EAppxBundle)
                {
                    return false;
                }

                return true;
            }

            bool IsFirstBetter(const MSStore::MSStoreDisplayCatalogPackage& first, const MSStore::MSStoreDisplayCatalogPackage& second) override
            {
                return IsPackageFormatBundle(first.PackageFormat) && !IsPackageFormatBundle(second.PackageFormat);
            }
        };

        struct LanguageFilter : public details::MSStorePackageComparisonField
        {
            LanguageFilter(std::vector<std::string> preference, std::vector<std::string> requirement) :
                details::MSStorePackageComparisonField("Language"), m_preference(std::move(preference)), m_requirement(std::move(requirement))
            {
                AICLI_LOG(CLI, Verbose,
                    << "Language Comparator created with Required Locales: " << Utility::ConvertContainerToString(m_requirement)
                    << " , Preferred Locales: " << Utility::ConvertContainerToString(m_preference));
            }

            static std::unique_ptr<LanguageFilter> Create(const std::vector<std::string>& requiredLanguages)
            {
                // Don't forget to add required lanugages when you create the filter.
                std::vector<std::string> requirement = requiredLanguages;
                if (requirement.empty())
                {
                    requirement = Settings::User().Get<Settings::Setting::InstallLocaleRequirement>();
                }

                std::vector<std::string> preference = Settings::User().Get<Settings::Setting::InstallLocalePreference>();
                if (preference.empty())
                {
                    preference = AppInstaller::Locale::GetUserPreferredLanguages();
                }

                if (!preference.empty() || !requirement.empty())
                {
                    return std::make_unique<LanguageFilter>(preference, requirement);
                }
                else
                {
                    return {};
                }
            }

            bool IsApplicable(const MSStore::MSStoreDisplayCatalogPackage& package) override
            {
                if (!m_requirement.empty())
                {
                    for (auto const& requiredLanguage : m_requirement)
                    {
                        for (auto const& supportedLanguage : package.Languages)
                        {
                            if (Locale::GetDistanceOfLanguage(requiredLanguage, supportedLanguage) >= Locale::MinimumDistanceScoreAsPerfectMatch)
                            {
                                return true;
                            }
                        }
                    }

                    return false;
                }
                else if (!m_preference.empty())
                {
                    for (auto const& preferredLanguage : m_preference)
                    {
                        for (auto const& supportedLanguage : package.Languages)
                        {
                            if (Locale::GetDistanceOfLanguage(preferredLanguage, supportedLanguage) >= Locale::MinimumDistanceScoreAsPerfectMatch)
                            {
                                return true;
                            }
                        }
                    }

                    return false;
                }
                else
                {
                    return true;
                }
            }

            bool IsFirstBetter(const MSStore::MSStoreDisplayCatalogPackage& first, const MSStore::MSStoreDisplayCatalogPackage& second)
            {
                if (m_preference.empty())
                {
                    return false;
                }

                for (auto const& preferredLocale : m_preference)
                {
                    double firstScore{};
                    double secondScore{};

                    // Find the best distance score of each package and compare the two. (probably turn this into a private method.)
                    for (auto const& supportedLanguage : first.Languages)
                    {
                        double currentScore = Locale::GetDistanceOfLanguage(preferredLocale, supportedLanguage);
                        if (currentScore > firstScore)
                        {
                            firstScore = currentScore;
                        }
                    }

                    for (auto const& supportedLanguage : second.Languages)
                    {
                        double currentScore = Locale::GetDistanceOfLanguage(preferredLocale, supportedLanguage);
                        if (currentScore > firstScore)
                        {
                            secondScore = currentScore;
                        }
                    }

                    if (firstScore >= Locale::MinimumDistanceScoreAsCompatibleMatch || secondScore >= Locale::MinimumDistanceScoreAsCompatibleMatch)
                    {
                        return (firstScore > secondScore);
                    }
                }

                return false;
            }

        private:
            std::vector<std::string> m_preference;
            std::vector<std::string> m_requirement;
        };

        struct ArchitectureFilter : public details::MSStorePackageComparisonField
        {
            ArchitectureFilter(std::vector<Utility::Architecture> required, std::vector<Utility::Architecture> preferred) :
                details::MSStorePackageComparisonField("Architecture"), m_required(std::move(required)), m_preferred(std::move(preferred))
            {
                AICLI_LOG(Core, Verbose,
                    << "Architecture Comparator created with required archs: " << Utility::ConvertContainerToString(m_required, Utility::ToString)
                    << " , Preferred archs: " << Utility::ConvertContainerToString(m_preferred, Utility::ToString));
            }

            static std::unique_ptr<ArchitectureFilter> Create(const std::vector<Utility::Architecture>& allowedArchitectures)
            {
                std::vector<Utility::Architecture> requiredArchitectures = allowedArchitectures;

                if (requiredArchitectures.empty())
                {
                    requiredArchitectures = Settings::User().Get<Settings::Setting::InstallArchitectureRequirement>();
                }

                std::vector<Utility::Architecture> optionalArchitectures = Settings::User().Get<Settings::Setting::InstallArchitecturePreference>();

                if (!requiredArchitectures.empty() || !optionalArchitectures.empty())
                {
                    return std::make_unique<ArchitectureFilter>(std::move(requiredArchitectures), std::move(optionalArchitectures));
                }
                else
                {
                    return {};
                }
            }

            bool IsApplicable(const MSStore::MSStoreDisplayCatalogPackage& package) override
            {
                if (!m_required.empty())
                {
                    return HasIntersection(m_required, package.Architectures);
                }

                return true;
            }

            bool IsFirstBetter(const MSStore::MSStoreDisplayCatalogPackage& first, const MSStore::MSStoreDisplayCatalogPackage& second) override
            {
                if (!m_preferred.empty())
                {
                    return (HasIntersection(first.Architectures, m_preferred) && !HasIntersection(second.Architectures, m_preferred));
                }

                return false;
            }

        private:
            bool HasIntersection(const std::vector<Utility::Architecture>& firstList, const std::vector<Utility::Architecture>& secondList)
            {
                for (auto arch : firstList)
                {
                    if (IsArchitectureInList(arch, secondList))
                    {
                        return true;
                    }
                }

                return false;
            }

            bool IsArchitectureInList(Utility::Architecture arch, const std::vector<Utility::Architecture>& architectureList)
            {
                return architectureList.end() != std::find_if(
                    architectureList.begin(),
                    architectureList.end(),
                    [&](const auto& a) { return a == arch; });
            }

            std::vector<Utility::Architecture> m_required;
            std::vector<Utility::Architecture> m_preferred;
        };
    }

    DisplayCatalogPackageComparator::DisplayCatalogPackageComparator(const std::vector<std::string>& requiredLanguages, const std::vector<Utility::Architecture>& requiredArchs)
    {
        AddComparator(std::make_unique<PackageFormatFilter>());
        AddComparator(LanguageFilter::Create(requiredLanguages));
        AddComparator(ArchitectureFilter::Create(requiredArchs));
    }

    void DisplayCatalogPackageComparator::AddComparator(std::unique_ptr<details::MSStorePackageComparisonField>&& comparator)
    {
        if (comparator)
        {
            m_comparators.emplace_back(std::move(comparator));
        }
    }

    std::optional<MSStore::MSStoreDisplayCatalogPackage> DisplayCatalogPackageComparator::GetPreferredPackage(const std::vector<MSStore::MSStoreDisplayCatalogPackage>& packages)
    {
        AICLI_LOG(CLI, Verbose, << "Starting MSStore package selection.");

        const MSStore::MSStoreDisplayCatalogPackage* result = nullptr;
        for (const auto& package : packages)
        {
            if (IsApplicable(package) && (!result || IsFirstBetter(package, *result)))
            {
                result = &package;
            }
        }

        if (result)
        {
            return *result;
        }
        else
        {
            return {};
        }
    }

    bool DisplayCatalogPackageComparator::IsFirstBetter(const MSStore::MSStoreDisplayCatalogPackage& first, const MSStore::MSStoreDisplayCatalogPackage& second)
    {
        for (const auto& comparator : m_comparators)
        {
            bool forwardCompare = comparator->IsFirstBetter(first, second);
            bool reverseCompare = comparator->IsFirstBetter(second, first);

            if (forwardCompare && reverseCompare)
            {
                AICLI_LOG(CLI, Error, << "Packages are both better than each other?");
                THROW_HR(E_UNEXPECTED);
            }

            if (forwardCompare && !reverseCompare)
            {
                return true;
            }
        }

        AICLI_LOG(CLI, Verbose, << "Packages are equivalent in priority");
        return false;
    }

    bool DisplayCatalogPackageComparator::IsApplicable(const MSStore::MSStoreDisplayCatalogPackage& package)
    {
        for (const auto& comparator : m_comparators)
        {
            bool result = comparator->IsApplicable(package);
            if (!result)
            {
                return false;
            }
        }

        return true;
    }
}
