// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/PackageVersionSelection.h"
#include "Public/winget/RepositorySource.h"
#include "Public/winget/PinningData.h"


namespace AppInstaller::Repository
{
    namespace
    {
        std::shared_ptr<IPackage> GetAvailablePackageFromSource(const std::vector<std::shared_ptr<IPackage>>& packages, const std::string_view sourceIdentifier)
        {
            for (const std::shared_ptr<IPackage>& package : packages)
            {
                if (sourceIdentifier == package->GetSource().GetIdentifier())
                {
                    return package;
                }
            }

            return {};
        }

        struct AvailablePackageVersionCollection : public IPackageVersionCollection
        {
            AvailablePackageVersionCollection(const std::shared_ptr<ICompositePackage>& composite, const std::shared_ptr<IPackageVersion>& installedVersion) :
                m_packages(composite->GetAvailable())
            {
                if (!installedVersion)
                {
                    return;
                }

                m_channel = installedVersion->GetProperty(PackageVersionProperty::Channel);

                // Remove the packages that are not from the installed source.
                Source installedVersionSource = installedVersion->GetSource();
                if (installedVersionSource && installedVersionSource.ContainsAvailablePackages())
                {
                    m_packages.erase(std::remove_if(m_packages.begin(), m_packages.end(), [&](const std::shared_ptr<IPackage>& p) { return installedVersionSource != p->GetSource(); }), m_packages.end());
                }
            }

            std::vector<PackageVersionKey> GetVersionKeys() const override
            {
                std::vector<PackageVersionKey> result;

                for (const std::shared_ptr<IPackage>& package : m_packages)
                {
                    std::vector<PackageVersionKey> versionKeys = package->GetVersionKeys();
                    std::copy(versionKeys.begin(), versionKeys.end(), std::back_inserter(result));
                }

                // Remove all elements whose channel does not match the installed package.
                if (m_channel)
                {
                    result.erase(
                        std::remove_if(result.begin(), result.end(), [&](const PackageVersionKey& pvk) { return !Utility::ICUCaseInsensitiveEquals(pvk.Channel, m_channel.value()); }),
                        result.end());
                }

                // Put latest versions at the front; for versions available from multiple sources maintain the order they were added in
                std::stable_sort(result.begin(), result.end());

                return result;
            }

            std::shared_ptr<IPackageVersion> GetVersion(const PackageVersionKey& versionKey) const override
            {
                // If there is a specific source, just use that package's result
                std::shared_ptr<IPackage> package;

                if (!versionKey.SourceId.empty())
                {
                    package = GetAvailablePackageFromSource(m_packages, versionKey.SourceId);
                }
                else if (m_packages.size() == 1)
                {
                    package = m_packages[0];
                }
                else if (versionKey.IsDefaultLatest())
                {
                    std::shared_ptr<IPackageVersion> result;
                    Utility::Version resultVersion;

                    for (const auto& p : m_packages)
                    {
                        std::shared_ptr<IPackageVersion> latest = p->GetLatestVersion();
                        Utility::Version version { latest->GetProperty(PackageVersionProperty::Version) };

                        if (!result || resultVersion < version)
                        {
                            result = std::move(latest);
                            resultVersion = std::move(version);
                        }
                    }

                    return result;
                }
                else
                {
                    // Otherwise, find the first version that matches
                    std::vector<PackageVersionKey> versions = GetVersionKeys();

                    for (const PackageVersionKey& key : versions)
                    {
                        if (key.IsMatch(versionKey))
                        {
                            package = GetAvailablePackageFromSource(m_packages, key.SourceId);
                            break;
                        }
                    }
                }

                return package ? package->GetVersion(versionKey) : nullptr;
            }

            std::shared_ptr<IPackageVersion> GetLatestVersion() const override
            {
                return GetVersion({ "", "", m_channel.value_or("") });
            }

        private:
            std::optional<std::string> m_channel;
            std::vector<std::shared_ptr<IPackage>> m_packages;
        };
    }

    std::shared_ptr<IPackageVersionCollection> GetAvailableVersionsForInstalledVersion(const std::shared_ptr<ICompositePackage>& composite)
    {
        return std::make_shared<AvailablePackageVersionCollection>(composite, GetInstalledVersion(composite));
    }

    std::shared_ptr<IPackageVersionCollection> GetAvailableVersionsForInstalledVersion(
        const std::shared_ptr<ICompositePackage>& composite,
        const std::shared_ptr<IPackageVersion>& installedVersion)
    {
        return std::make_shared<AvailablePackageVersionCollection>(composite, installedVersion);
    }

    std::shared_ptr<IPackageVersionCollection> GetAllAvailableVersions(const std::shared_ptr<ICompositePackage>& composite)
    {
        return GetAvailableVersionsForInstalledVersion(composite, nullptr);
    }

    std::shared_ptr<IPackageVersion> GetInstalledVersion(const std::shared_ptr<ICompositePackage>& composite)
    {
        auto installedPackage = composite->GetInstalled();
        return installedPackage ? installedPackage->GetLatestVersion() : nullptr;
    }

    std::shared_ptr<IPackage> GetAvailablePackageFromSource(const std::shared_ptr<ICompositePackage>& composite, const std::string_view sourceIdentifier)
    {
        return GetAvailablePackageFromSource(composite->GetAvailable(), sourceIdentifier);
    }

    LatestApplicableVersionData GetLatestApplicableVersion(const std::shared_ptr<ICompositePackage>& composite)
    {
        using namespace AppInstaller::Pinning;

        LatestApplicableVersionData result;

        auto installedVersion = AppInstaller::Repository::GetInstalledVersion(composite);
        auto availableVersions = AppInstaller::Repository::GetAvailableVersionsForInstalledVersion(composite, installedVersion);

        PinningData pinningData{ PinningData::Disposition::ReadOnly };
        auto evaluator = pinningData.CreatePinStateEvaluator(PinBehavior::ConsiderPins, installedVersion);

        AppInstaller::Manifest::ManifestComparator::Options options;
        if (installedVersion)
        {
            GetManifestComparatorOptionsFromMetadata(options, installedVersion->GetMetadata());
        }
        AppInstaller::Manifest::ManifestComparator manifestComparator{ options };

        auto availableVersionKeys = availableVersions->GetVersionKeys();
        for (const auto& availableVersionKey : availableVersionKeys)
        {
            auto availableVersion = availableVersions->GetVersion(availableVersionKey);

            if (installedVersion && !evaluator.IsUpdate(availableVersion))
            {
                // Version too low or different channel for upgrade
                continue;
            }

            if (evaluator.EvaluatePinType(availableVersion) != AppInstaller::Pinning::PinType::Unknown)
            {
                // Pinned
                continue;
            }

            auto manifestComparatorResult = manifestComparator.GetPreferredInstaller(availableVersion->GetManifest());
            if (!manifestComparatorResult.installer.has_value())
            {
                // No applicable installer
                continue;
            }

            result.LatestApplicableVersion = availableVersion;
            if (installedVersion)
            {
                result.UpdateAvailable = true;
            }

            break;
        }

        return result;
    }

    void GetManifestComparatorOptionsFromMetadata(AppInstaller::Manifest::ManifestComparator::Options& options, const IPackageVersion::Metadata& metadata, bool includeAllowedArchitectures)
    {
        auto installedTypeItr = metadata.find(Repository::PackageVersionMetadata::InstalledType);
        if (installedTypeItr != metadata.end())
        {
            options.CurrentlyInstalledType = Manifest::ConvertToInstallerTypeEnum(installedTypeItr->second);
        }

        auto installedScopeItr = metadata.find(Repository::PackageVersionMetadata::InstalledScope);
        if (installedScopeItr != metadata.end())
        {
            options.CurrentlyInstalledScope = Manifest::ConvertToScopeEnum(installedScopeItr->second);
        }

        auto userIntentLocaleItr = metadata.find(Repository::PackageVersionMetadata::UserIntentLocale);
        if (userIntentLocaleItr != metadata.end())
        {
            options.PreviousUserIntentLocale = userIntentLocaleItr->second;
        }

        auto installedLocaleItr = metadata.find(Repository::PackageVersionMetadata::InstalledLocale);
        if (installedLocaleItr != metadata.end())
        {
            options.CurrentlyInstalledLocale = installedLocaleItr->second;
        }

        if (includeAllowedArchitectures)
        {
            auto userIntentItr = metadata.find(Repository::PackageVersionMetadata::UserIntentArchitecture);
            if (userIntentItr != metadata.end())
            {
                // For upgrade, user intent from previous install is considered requirement
                options.AllowedArchitectures.emplace_back(Utility::ConvertToArchitectureEnum(userIntentItr->second));
            }
            else
            {
                auto installedItr = metadata.find(Repository::PackageVersionMetadata::InstalledArchitecture);
                if (installedItr != metadata.end())
                {
                    // For upgrade, previous installed architecture should be considered first preference and is always allowed.
                    // Then check settings requirements and preferences.
                    options.AllowedArchitectures.emplace_back(Utility::ConvertToArchitectureEnum(installedItr->second));
                }

                std::vector<Utility::Architecture> requiredArchitectures = Settings::User().Get<Settings::Setting::InstallArchitectureRequirement>();
                std::vector<Utility::Architecture> optionalArchitectures = Settings::User().Get<Settings::Setting::InstallArchitecturePreference>();

                if (!requiredArchitectures.empty())
                {
                    // Required architecture list from settings if applicable
                    options.AllowedArchitectures.insert(options.AllowedArchitectures.end(), requiredArchitectures.begin(), requiredArchitectures.end());
                }
                else
                {
                    // Preferred architecture list from settings if applicable, add Unknown to indicate allowing remaining applicable
                    if (!optionalArchitectures.empty())
                    {
                        options.AllowedArchitectures.insert(options.AllowedArchitectures.end(), optionalArchitectures.begin(), optionalArchitectures.end());
                    }

                    options.AllowedArchitectures.emplace_back(Utility::Architecture::Unknown);
                }
            }
        }
    }

    std::optional<int32_t> GetSourcePriority(const std::shared_ptr<ICompositePackage>& composite)
    {
        auto installed = composite->GetInstalled();

        if (installed)
        {
            auto installedVersion = installed->GetLatestVersion();

            if (installedVersion)
            {
                auto installedSource = installedVersion->GetSource();

                if (installedSource.ContainsAvailablePackages())
                {
                    return installedSource.GetDetails().Priority;
                }
            }
        }

        auto available = composite->GetAvailable();

        if (!available.empty())
        {
            return available.front()->GetSource().GetDetails().Priority;
        }

        return std::nullopt;
    }
}
