// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>
#include <winget/Compression.h>


namespace AppInstaller::Manifest
{
    // Contains the manifest that stores package version data for index v2
    struct PackageVersionDataManifest
    {
        // The file name to use for the package version data manifest.
        static std::string_view VersionManifestFileName();

        // The file name to use for the compressed package version data manifest.
        static std::string_view VersionManifestCompressedFileName();

        // Creates the compressor used by the PackageVersionDataManifest.
        static Compression::Compressor CreateCompressor();

        // Creates the decompressor used by the PackageVersionDataManifest.
        static Compression::Decompressor CreateDecompressor();

        // Data on an individual version.
        struct VersionData
        {
            VersionData(
                const Utility::VersionAndChannel& versionAndChannel,
                std::optional<std::string> arpMinVersion,
                std::optional<std::string> arpMaxVersion,
                std::optional<std::string> relativePath,
                std::optional<std::string> manifestHash);

            Utility::Version Version;
            std::optional<std::string> ArpMinVersion;
            std::optional<std::string> ArpMaxVersion;
            std::string ManifestRelativePath;
            std::string ManifestHash;
        };

        // Adds the given version data to the manifest.
        void AddVersion(VersionData&& versionData);

        // Returns a serialized version of the current manifest data.
        std::string Serialize();

    private:
        std::vector<VersionData> m_versions;
    };
}
