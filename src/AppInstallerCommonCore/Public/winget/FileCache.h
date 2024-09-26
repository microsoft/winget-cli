// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerRuntime.h>
#include <AppInstallerSHA256.h>
#include <filesystem>
#include <istream>
#include <sstream>

namespace AppInstaller::Caching
{
    // A file cache for relatively small files (they are always full loaded into memory due to the hash enforcement).
    struct FileCache
    {
        // The supported file cache types.
        enum class Type
        {
            // Manifests for index V1.
            IndexV1_Manifest,
            // Package version data files for index V2.
            IndexV2_PackageVersionData,
            // Manifests for index V2.
            IndexV2_Manifest,
            // Icon for use during show command when sixel rendering is enabled.
            Icon,
#ifndef AICLI_DISABLE_TEST_HOOKS
            // The test type.
            Tests,
#endif
        };

        // Contains information about a specific file cache instance.
        struct Details
        {
            Details(Type type, std::string identifier);

            Runtime::PathName BasePath;
            Type Type;
            std::string Identifier;

            // Gets the full path to the cache directory.
            std::filesystem::path GetCachePath() const;
        };

        // Construct a file cache for the given instance.
        FileCache(Type type, std::string identifier, std::vector<std::string> sources);

        // Gets the details for this file cache.
        const Details& GetDetails() const;

        // Gets a stream containing the contents of the requested file.
        // The hash must match for this function to return successfully.
        std::unique_ptr<std::istream> GetFile(const std::filesystem::path& relativePath, const Utility::SHA256::HashBuffer& expectedHash) const;

    private:
        // Gets a stream containing the contents of the requested file from an upstream source.
        // The hash must match for this function to return successfully.
        std::unique_ptr<std::stringstream> GetUpstreamFile(std::string relativePath, const Utility::SHA256::HashBuffer& expectedHash) const;

        Details m_details;
        std::vector<std::string> m_sources;
        std::filesystem::path m_cacheBase;
    };
}
