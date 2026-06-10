// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SourceFactory.h"

using namespace std::string_view_literals;


namespace AppInstaller::Repository
{
    // Enable test to inject a custom tracking source by integrating the internal source creation into the standard flows.
    // If overriding, the opened ISource must be a SQLiteIndexSource or the code will fail.
    struct PackageTrackingCatalogSourceFactory
    {
        // Get the type string for this source.
        static constexpr std::string_view Type()
        {
            using namespace std::string_view_literals;
            return "Microsoft.PackageTracking"sv;
        }

        // Creates a source factory for this type.
        static std::unique_ptr<ISourceFactory> Create();
    };
}
