// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"
#include "ExecutionArgs.h"
#include <winget/Manifest.h>
#include <winget/RepositorySearch.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::CLI::Workflow
{
    // Flags to indicate why an installer was not applicable
    enum class InapplicabilityFlags : int
    {
        None = 0x0,
        OSVersion = 0x1,
        InstalledScope = 0x2,
        InstalledType = 0x4,
        InstalledLocale = 0x8,
        Locale = 0x10,
        Scope = 0x20,
        MachineArchitecture = 0x40,
        Market = 0x80,
        InstallerType = 0x100,
    };

    DEFINE_ENUM_FLAG_OPERATORS(InapplicabilityFlags);

    namespace details
    {
        // An interface for defining new filters based on user inputs.
        struct FilterField
        {
            FilterField(std::string_view name) : m_name(name) {}

            virtual ~FilterField() = default;

            std::string_view Name() const { return m_name; }

            // Determines if the installer is applicable based on this field alone.
            virtual InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer) = 0;

            // Explains why the filter regarded this installer as inapplicable.
            // Will only be called when IsApplicable returns false.
            virtual std::string ExplainInapplicable(const Manifest::ManifestInstaller& installer) = 0;

        private:
            std::string_view m_name;
        };

        // The result of ComparisonField::IsFirstBetter
        enum class ComparisonResult
        {
            // The first input is not better than the second input.
            Negative,
            // The first input is somewhat better than the second input.
            // If another comparison has a strong positive result, it will override a weak result.
            WeakPositive,
            // The first input is definitely better than the second input.
            StrongPositive,
        };

        // An interface for defining new comparisons based on user inputs.
        struct ComparisonField : public FilterField
        {
            using FilterField::FilterField;

            virtual ~ComparisonField() = default;

            // Determines if the first installer is a better choice based on this field alone.
            virtual ComparisonResult IsFirstBetter(const Manifest::ManifestInstaller& first, const Manifest::ManifestInstaller& second) = 0;
        };
    }

    struct InstallerAndInapplicabilities
    {
        std::optional<Manifest::ManifestInstaller> installer;
        std::vector<InapplicabilityFlags> inapplicabilitiesInstaller;
    };

    // Class in charge of comparing manifest entries
    struct ManifestComparator
    {
        ManifestComparator(const Execution::Context& context, const Repository::IPackageVersion::Metadata& installationMetadata);

        // Gets the best installer from the manifest, if at least one is applicable.
        InstallerAndInapplicabilities GetPreferredInstaller(const Manifest::Manifest& manifest);

        // Determines if an installer is applicable.
        InapplicabilityFlags IsApplicable(const Manifest::ManifestInstaller& installer);

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