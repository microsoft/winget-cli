// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/AppInstallerRepositorySource.h"
#include "SourceFactory.h"

#include <string_view>

namespace AppInstaller::Repository::Microsoft
{
    using namespace std::string_view_literals;

    // A source of installed packages on the local system.
    // Arg  ::  A value indicating how the list is to be filtered.
    // Data ::  Not used.
    struct PredefinedInstalledSourceFactory
    {
        // Get the type string for this source.
        static constexpr std::string_view Type()
        {
            return "Microsoft.Predefined.Installed"sv;
        }

        // The filtering level for the source.
        enum class Filter
        {
            None,
            ARP_System,
            ARP_User,
            MSIX,
        };

        // Converts a filter to its string.
        static std::string_view FilterToString(Filter filter);

        // Converts a string to its filter value.
        static Filter StringToFilter(std::string_view filter);

        // Creates a source factory for this type.
        static std::unique_ptr<ISourceFactory> Create();
    };
}
