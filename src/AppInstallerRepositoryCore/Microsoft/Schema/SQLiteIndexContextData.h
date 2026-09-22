// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLanguageUtilities.h>
#include <filesystem>
#include <string>


namespace AppInstaller::Repository::Microsoft::Schema
{
    // Names a property
    enum class Property : size_t
    {
        PackageUpdateTrackingBaseTime,
        IntermediateFileOutputPath,
        DatabaseFilePath,
        DeltaBaselineIndexPath,
        DeltaMarkAsBaseline,
        DeltaOutputPath,
        DeltaBaselineRelativeSourcePath,
        DeltaBaselinePackageVersion,
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

        template <>
        struct PropertyMapping<Property::DeltaBaselineIndexPath>
        {
            using value_t = std::filesystem::path;
            static constexpr bool SetThroughInterface = false;
        };

        // Designates the index being prepared as a baseline, in place of naming an existing one.
        // The only meaningful value is true; an index is either being designated or it is not.
        template <>
        struct PropertyMapping<Property::DeltaMarkAsBaseline>
        {
            using value_t = bool;
            static constexpr bool SetThroughInterface = false;
        };

        template <>
        struct PropertyMapping<Property::DeltaOutputPath>
        {
            using value_t = std::filesystem::path;
            static constexpr bool SetThroughInterface = false;
        };

        // Not a local path; the location of the baseline package relative to the source's base
        // location, which only the consuming client can resolve.
        template <>
        struct PropertyMapping<Property::DeltaBaselineRelativeSourcePath>
        {
            using value_t = std::string;
            static constexpr bool SetThroughInterface = false;
        };

        template <>
        struct PropertyMapping<Property::DeltaBaselinePackageVersion>
        {
            using value_t = std::string;
            static constexpr bool SetThroughInterface = false;
        };
    }

    using SQLiteIndexContextData = EnumBasedVariantMap<Property, details::PropertyMapping>;
}
