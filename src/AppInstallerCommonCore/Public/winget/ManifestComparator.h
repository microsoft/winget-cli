// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Manifest.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace AppInstaller::Manifest
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
            virtual InapplicabilityFlags IsApplicable(const AppInstaller::Manifest::ManifestInstaller& installer) = 0;

            // Explains why the filter regarded this installer as inapplicable.
            // Will only be called when IsApplicable returns false.
            virtual std::string ExplainInapplicable(const AppInstaller::Manifest::ManifestInstaller& installer) = 0;

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
            virtual ComparisonResult IsFirstBetter(const AppInstaller::Manifest::ManifestInstaller& first, const AppInstaller::Manifest::ManifestInstaller& second) = 0;
        };
    }

    struct InstallerAndInapplicabilities
    {
        std::optional<AppInstaller::Manifest::ManifestInstaller> installer;
        std::vector<InapplicabilityFlags> inapplicabilitiesInstaller;
    };

    // Class in charge of comparing manifest entries
    struct ManifestComparator
    {
        // Options that affect the comparisons.
        struct Options
        {
            // The allowed architectures and a value indicating whether to perform applicability checks.
            std::vector<Utility::Architecture> AllowedArchitectures;
            bool SkipApplicabilityCheck = false;

            // The requested installer type.
            std::optional<InstallerTypeEnum> RequestedInstallerType;

            // The currently installed type.
            std::optional<InstallerTypeEnum> CurrentlyInstalledType;

            // The requested installer scope and a value indicating whether and unknown scope is acceptable.
            std::optional<ScopeEnum> RequestedInstallerScope;
            std::optional<bool> AllowUnknownScope;

            // The currently installed scope.
            std::optional<ScopeEnum> CurrentlyInstalledScope;

            // The requested installer locale.
            std::optional<std::string> RequestedInstallerLocale;

            // Get the currently installed locale intent and value.
            std::optional<std::string> PreviousUserIntentLocale;
            std::optional<std::string> CurrentlyInstalledLocale;
        };

        ManifestComparator(const Options& options);

        // Gets the best installer from the manifest, if at least one is applicable.
        InstallerAndInapplicabilities GetPreferredInstaller(const AppInstaller::Manifest::Manifest& manifest);

        // Determines if an installer is applicable.
        InapplicabilityFlags IsApplicable(const AppInstaller::Manifest::ManifestInstaller& installer);

        // Determines if the first installer is a better choice.
        bool IsFirstBetter(
            const AppInstaller::Manifest::ManifestInstaller& first,
            const AppInstaller::Manifest::ManifestInstaller& second);

    private:
        void AddFilter(std::unique_ptr<details::FilterField>&& filter);
        void AddComparator(std::unique_ptr<details::ComparisonField>&& comparator);

        std::vector<std::unique_ptr<details::FilterField>> m_filters;
        // Non-owning pointers to values in m_filters.
        std::vector<details::ComparisonField*> m_comparators;
    };

}
