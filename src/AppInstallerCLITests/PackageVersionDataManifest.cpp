// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/PackageVersionDataManifest.h>

using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility;

void RequireVersionDataEqual(const PackageVersionDataManifest::VersionData& first, const PackageVersionDataManifest::VersionData& second)
{
    REQUIRE(first.Version == second.Version);
    REQUIRE(first.ArpMinVersion == second.ArpMinVersion);
    REQUIRE(first.ArpMaxVersion == second.ArpMaxVersion);
    REQUIRE(first.ManifestRelativePath == second.ManifestRelativePath);
    REQUIRE(first.ManifestHash == second.ManifestHash);
}

TEST_CASE("PackageVersionDataManifest_Empty", "[PackageVersionDataManifest]")
{
    PackageVersionDataManifest original;

    PackageVersionDataManifest copy;
    copy.Deserialize(original.Serialize());

    REQUIRE(original.Versions().empty());
    REQUIRE(copy.Versions().empty());
}

TEST_CASE("PackageVersionDataManifest_Single_Simple", "[PackageVersionDataManifest]")
{
    PackageVersionDataManifest original;
    original.AddVersion({ VersionAndChannel{ Version{ "1.0" }, Channel{} }, {}, {}, "path", "hash" });

    PackageVersionDataManifest copy;
    copy.Deserialize(original.Serialize());

    REQUIRE(original.Versions().size() == 1);
    REQUIRE(copy.Versions().size() == 1);

    RequireVersionDataEqual(copy.Versions()[0], original.Versions()[0]);
}

TEST_CASE("PackageVersionDataManifest_Single_Complete", "[PackageVersionDataManifest]")
{
    PackageVersionDataManifest original;
    original.AddVersion({ VersionAndChannel{ Version{ "1.0" }, Channel{} }, ".99", "1.01", "path", "hash"});

    PackageVersionDataManifest copy;
    copy.Deserialize(original.Serialize());

    REQUIRE(original.Versions().size() == 1);
    REQUIRE(copy.Versions().size() == 1);

    RequireVersionDataEqual(copy.Versions()[0], original.Versions()[0]);
}

TEST_CASE("PackageVersionDataManifest_Multiple", "[PackageVersionDataManifest]")
{
    PackageVersionDataManifest original;
    original.AddVersion({ VersionAndChannel{ Version{ "1.0" }, Channel{} }, ".99", "1.01", "path", "hash" });
    original.AddVersion({ VersionAndChannel{ Version{ "1.1" }, Channel{} }, "1.99", "2.01", "path2", "hash2" });
    original.AddVersion({ VersionAndChannel{ Version{ "1.2" }, Channel{} }, {}, {}, "path2", "hash2" });
    original.AddVersion({ VersionAndChannel{ Version{ "2.0" }, Channel{} }, "3.99", "15.01", "path4", "hash4" });

    PackageVersionDataManifest copy;
    copy.Deserialize(original.Serialize());

    REQUIRE(original.Versions().size() == copy.Versions().size());

    for (size_t i = 0; i < original.Versions().size(); ++i)
    {
        INFO(i);
        RequireVersionDataEqual(copy.Versions()[i], original.Versions()[i]);
    }
}

TEST_CASE("PackageVersionDataManifest_CompressionRoundTrip", "[PackageVersionDataManifest]")
{
    PackageVersionDataManifest original;
    original.AddVersion({ VersionAndChannel{ Version{ "1.0" }, Channel{} }, ".99", "1.01", "path", "hash" });
    original.AddVersion({ VersionAndChannel{ Version{ "1.1" }, Channel{} }, "1.99", "2.01", "path2", "hash2" });
    original.AddVersion({ VersionAndChannel{ Version{ "1.2" }, Channel{} }, {}, {}, "path2", "hash2" });
    original.AddVersion({ VersionAndChannel{ Version{ "2.0" }, Channel{} }, "3.99", "15.01", "path4", "hash4" });

    std::string serialized = original.Serialize();
    auto compressed = PackageVersionDataManifest::CreateCompressor().Compress(serialized);

    auto decompressed = PackageVersionDataManifest::CreateDecompressor().Decompress(compressed);

    PackageVersionDataManifest copy;
    copy.Deserialize(decompressed);

    REQUIRE(original.Versions().size() == copy.Versions().size());

    for (size_t i = 0; i < original.Versions().size(); ++i)
    {
        INFO(i);
        RequireVersionDataEqual(copy.Versions()[i], original.Versions()[i]);
    }
}
