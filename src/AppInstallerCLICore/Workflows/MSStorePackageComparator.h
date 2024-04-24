// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"
#include "ExecutionArgs.h"
#include <winget/MSStoreDownload.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace AppInstaller::CLI::Workflow
{
    namespace details
    {
        // An interface for defining new comparisons based on user inputs.
        struct MSStorePackageComparisonField
        {
            MSStorePackageComparisonField(std::string_view name) : m_name(name) {}

            virtual ~MSStorePackageComparisonField() = default;

            std::string_view Name() const { return m_name; }

            virtual bool IsApplicable(const MSStore::MSStoreDisplayCatalogPackage& package) = 0;

            virtual bool IsFirstBetter(const MSStore::MSStoreDisplayCatalogPackage& first, const MSStore::MSStoreDisplayCatalogPackage& second) = 0;

        private:
            std::string_view m_name;
        };
    }

    // Class in charge of comparing manifest entries
    struct DisplayCatalogPackageComparator
    {
        DisplayCatalogPackageComparator(const std::vector<std::string>& requiredLanguages, const std::vector<Utility::Architecture>& requiredArchs);

        // Gets the best installer from the manifest, if at least one is applicable.
        std::optional<MSStore::MSStoreDisplayCatalogPackage> GetPreferredPackage(const std::vector<MSStore::MSStoreDisplayCatalogPackage>& package);

        // Determines if an installer is applicable.
        bool IsApplicable(const MSStore::MSStoreDisplayCatalogPackage& package);

        //// Determines if the first installer is a better choice.
        bool IsFirstBetter(
            const MSStore::MSStoreDisplayCatalogPackage& first,
            const MSStore::MSStoreDisplayCatalogPackage& second);

    private:
        void AddComparator(std::unique_ptr<details::MSStorePackageComparisonField>&& comparator);

        std::vector<std::unique_ptr<details::MSStorePackageComparisonField>> m_comparators;
    };
}
