// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLanguageUtilities.h>
#include <filesystem>


namespace AppInstaller::Repository::Microsoft::Schema
{
    // Names a property
    enum class Property : size_t
    {
        PackageUpdateTrackingBaseTime,
        IntermediateFileOutputPath,
        DatabaseFilePath,
        Max
    };

    namespace details
    {
        template <Property D>
        struct PropertyMapping
        {
            // value_t type specifies the type of this property
        };

        template <>
        struct PropertyMapping<Property::PackageUpdateTrackingBaseTime>
        {
            using value_t = int64_t;
            static constexpr bool SetThroughInterface = true;
        };

        template <>
        struct PropertyMapping<Property::IntermediateFileOutputPath>
        {
            using value_t = std::filesystem::path;
            static constexpr bool SetThroughInterface = false;
        };

        template <>
        struct PropertyMapping<Property::DatabaseFilePath>
        {
            using value_t = std::filesystem::path;
            static constexpr bool SetThroughInterface = false;
        };
    }

    using SQLiteIndexContextData = EnumBasedVariantMap<Property, details::PropertyMapping>;
}
