// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionArgs.h"
#include <winget/Manifest.h>
#include <AppInstallerRepositorySearch.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::CLI::Workflow
{
    namespace details
    {
        // An interface for defining new filters based on user inputs.
        struct FilterField
        {
            FilterField(std::string_view name) : m_name(name) {}

            virtual ~FilterField() = default;

            std::string_view Name() const { return m_name; }

            // Determines if the installer is applicable based on this field alone.
            virtual bool IsApplicable(const Manifest::ManifestInstaller& installer) = 0;

            // Explains why the filter regarded this installer as inapplicable.
            // Will only be called when IsApplicable returns false.
            virtual std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) = 0;

        private:
            std::string_view m_name;
        };

        // An interface for defining new comparisons based on user inputs.
        struct ComparisonField : public FilterField
        {
            using FilterField::FilterField;

            virtual ~ComparisonField() = default;

            // Determines if the first installer is a better choice based on this field alone.
            virtual bool IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) = 0;
        };
    }

    // Class in charge of comparing manifest entries
    struct ManifestComparator
    {
        ManifestComparator(const Execution::Args&, const Repository::IPackageVersion::Metadata& installationMetadata);

        // Gets the best installer from the manifest, if at least one is applicable.
        std::optional<Manifest::ManifestInstaller> GetPreferredInstaller(const Manifest::Manifest& manifest);

        // Determines if an installer is applicable.
        bool IsApplicable(const Manifest::ManifestInstaller& installer);

        // Determines if the first installer is a better choice.
        bool IsFirstBetter(
            const Manifest::ManifestInstaller& first,
            const Manifest::ManifestInstaller& second);

    private:
        void AddFilter(std::unique_ptr<details::FilterField>&& filter);
        void AddComparator(std::unique_ptr<details::ComparisonField>&& comparator);

        std::vector<std::unique_ptr<details::FilterField>> m_filters;
        // Non-owning pointers to values in m_filters.
        std::vector<details::ComparisonField*> m_comparators;
    };

}