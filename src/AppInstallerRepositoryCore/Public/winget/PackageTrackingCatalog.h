// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <winget/Manifest.h>

#include <memory>


namespace AppInstaller::Repository
{
    struct Source;
    struct SearchRequest;
    struct SearchResult;
    enum class PackageVersionMetadata;

    // A catalog for tracking package actions from a given source.
    struct PackageTrackingCatalog
    {
        friend Source;

        PackageTrackingCatalog();
        PackageTrackingCatalog(const PackageTrackingCatalog&);
        PackageTrackingCatalog& operator=(const PackageTrackingCatalog&);
        PackageTrackingCatalog(PackageTrackingCatalog&&) noexcept;
        PackageTrackingCatalog& operator=(PackageTrackingCatalog&&) noexcept;
        ~PackageTrackingCatalog();

        // Removes the package tracking catalog for a given source identifier.
        static void RemoveForSource(const std::string& identifier);

        // Determines if the current object holds anything.
        operator bool() const;

        // Execute a search against the catalog.
        // Note that the packages in the results have the versions under "available" in order to
        // expose all versions contained therein (in the event that this is deemed useful at some point).
        SearchResult Search(const SearchRequest& request) const;

        // Enables more granular control over the metadata in the tracking catalog if necessary.
        struct Version
        {
            friend PackageTrackingCatalog;

            Version(const Version&);
            Version& operator=(const Version&);
            Version(Version&&) noexcept;
            Version& operator=(Version&&) noexcept;
            ~Version();

            // Set the given metadata value.
            void SetMetadata(PackageVersionMetadata metadata, const Utility::NormalizedString& value);

        private:
            struct implementation;
            Version(PackageTrackingCatalog& catalog, std::shared_ptr<implementation>&& value);
            std::shared_ptr<implementation> m_implementation;
            PackageTrackingCatalog& m_catalog;
        };

        // Records an installation of the given package.
        Version RecordInstall(Manifest::Manifest& manifest, const Manifest::ManifestInstaller& installer, bool isUpgrade);

        // Records an uninstall of the given package.
        void RecordUninstall(const Utility::LocIndString& packageIdentifier);

    protected:
        // Creates or opens the tracking catalog for the given source.
        static PackageTrackingCatalog CreateForSource(const Source& source);

    private:
        struct implementation;
        std::shared_ptr<implementation> m_implementation;
    };
}
