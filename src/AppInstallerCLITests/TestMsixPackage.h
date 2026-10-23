// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>
#include <string>

namespace TestCommon
{
    // Describes a source index package to build.
    struct TestIndexPackageDefinition
    {
        // The identity name. Together with the publisher this determines the package family name.
        std::string Name = "AppInstallerCLITestsFakeIndex";

        // Must be a well formed distinguished name; the publisher hash is computed from it.
        std::string Publisher = "CN=Code Sign Test (DO NOT TRUST), O=Microsoft Corporation, L=Redmond, S=Washington, C=US";

        // Four part version. This is what the source update path compares to decide whether it
        // already holds a package, so it is the field most tests will vary.
        std::string Version = "1.0.0.0";
    };

    // Builds an unsigned MSIX at outputPath carrying indexFile as `Public\index.db`, which is where
    // the pre-indexed source expects to find an index.
    //
    // Returns the package family name of the result, read back out of the package that was written
    // rather than computed.
    std::string CreateTestIndexPackage(
        const std::filesystem::path& outputPath,
        const std::filesystem::path& indexFile,
        const TestIndexPackageDefinition& definition = {});
}
