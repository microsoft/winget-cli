// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/PackageVersionSelection.h"
#include "Public/winget/RepositorySource.h"


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
}
