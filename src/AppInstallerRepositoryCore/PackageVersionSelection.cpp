// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/PackageVersionSelection.h"
#include "Public/winget/RepositorySource.h"


namespace AppInstaller::Repository
{
    namespace
    {
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

                // Remove the packages not from this source.
                Source installedVersionSource = installedVersion->GetSource();
                m_packages.erase(std::remove_if(m_packages.begin(), m_packages.end(), [&](const std::shared_ptr<IPackage>& p) { return installedVersionSource != p->GetSource(); }), m_packages.end());
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
                std::string_view channel = m_channel;
                result.erase(
                    std::remove_if(result.begin(), result.end(), [&](const PackageVersionKey& pvk) { return !Utility::ICUCaseInsensitiveEquals(pvk.Channel, channel); }),
                    result.end());

                return result;
            }

            std::shared_ptr<IPackageVersion> GetVersion(const PackageVersionKey& versionKey) const override
            {
                for (const std::shared_ptr<IPackage>& package : m_packages)
                {
                    if (!Utility::IsEmptyOrWhitespace(versionKey.SourceId))
                    {
                        if (versionKey.SourceId != package->GetSource().GetIdentifier())
                        {
                            continue;
                        }
                    }

                    return package->GetVersion(versionKey);
                }

                return {};
            }

            std::shared_ptr<IPackageVersion> GetLatestVersion() const override
            {
                return GetVersion({ "", "", m_channel });
            }

        private:
            std::string m_channel;
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

    std::shared_ptr<IPackageVersion> GetInstalledVersion(const std::shared_ptr<ICompositePackage>& composite)
    {
        auto installedPackage = composite->GetInstalled();
        return installedPackage ? installedPackage->GetLatestVersion() : nullptr;
    }

    std::shared_ptr<IPackage> GetAvailablePackageFromSource(const std::shared_ptr<ICompositePackage>& composite, const std::string_view sourceIdentifier)
    {
        for (const std::shared_ptr<IPackage>& package : composite->GetAvailable())
        {
            if (sourceIdentifier != package->GetSource().GetIdentifier())
            {
                return package;
            }
        }

        return {};
    }
}
