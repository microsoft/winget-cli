// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ISource.h"
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
            // Contains user ARP, machine ARP and user MSIX
            None,
            // Contains user ARP and machine ARP
            ARP,
            // Contains user MSIX
            MSIX,
            // Contains user ARP and user MSIX
            User,
            // Contains machine ARP and machine MSIX
            Machine,
            // Same as None but creating the source reference causes the next Open to always update the cache
            NoneWithForcedCacheUpdate,
        };

        // Converts a filter to its string.
        static std::string_view FilterToString(Filter filter);

        // Converts a string to its filter value.
        static Filter StringToFilter(std::string_view filter);

        // Creates a source factory for this type.
        static std::unique_ptr<ISourceFactory> Create();
    };
}
