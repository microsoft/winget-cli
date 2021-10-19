// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerRepositorySearch.h>
#include <AppInstallerRepositorySource.h>

#include <memory>


namespace AppInstaller::Repository
{
    // A catalog for tracking package actions from a given source.
    struct PackageTrackingCatalog
    {
        PackageTrackingCatalog();
        PackageTrackingCatalog(const PackageTrackingCatalog&);
        PackageTrackingCatalog& operator=(const PackageTrackingCatalog&);
        PackageTrackingCatalog(PackageTrackingCatalog&&) noexcept;
        PackageTrackingCatalog& operator=(PackageTrackingCatalog&&) noexcept;
        ~PackageTrackingCatalog();

        // Creates or opens the tracking catalog for the given source.
        // TODO: Make creation exclusive to the refactored Source type.
        static PackageTrackingCatalog CreateForSource(const std::shared_ptr<const ISource>& source);

        // Removes the package tracking catalog for a given source.
        static void RemoveForSource(const std::string& identifier);

        // Execute a search against the catalog.
        // Note that the pacakges in the results have the versions under "available" in order to
        // expose all versions contained therein (in the event that this is deemed useful at some point).
        SearchResult Search(const SearchRequest& request) const;

        // Enables more granular control over the metadata in the tracking catalog if necessary.
        struct Version
        {
            friend PackageTrackingCatalog;

            Version();
            Version(const Version&);
            Version& operator=(const Version&);
            Version(Version&&) noexcept;
            Version& operator=(Version&&) noexcept;
            ~Version();

            // Set the given metadata value.
            void SetMetadata(PackageVersionMetadata metadata, const Utility::NormalizedString& value);

        private:
            struct implementation;
            Version(std::shared_ptr<implementation>&& value);
            std::shared_ptr<implementation> m_implementation;
        };

        // Records an installation of the given package.
        Version RecordInstall(const Manifest::Manifest& manifest, const Manifest::ManifestInstaller& installer, bool isUpgrade);

        // Records an uninstall of the given package.
        void RecordUninstall(const Utility::LocIndString& packageIdentifier);

    private:
        struct implementation;
        std::shared_ptr<implementation> m_implementation;
    };
}
