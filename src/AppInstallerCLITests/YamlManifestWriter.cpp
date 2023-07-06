// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/ManifestYamlParser.h>
#include <winget/ManifestYamlWriter.h>
#include <winget/Yaml.h>

using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::YAML;

// Read in a Test manifest, convert to manifest, then convert to yaml and verify contents.
TEST_CASE("WriteYamlFromManifestAndVerifyContents", "[ManifestYamlWriter]")
{
    auto manifestFile = TestDataFile("ManifestV1_5-Singleton.yaml");
    Manifest manifest = YamlParser::CreateFromPath(manifestFile);

    const std::string& output = YamlWriter::ManifestYamlDepopulator::DepopulateManifest(manifest);

    TempDirectory tempDirectory{ "generatedFolder" };
    std::filesystem::path out = tempDirectory.GetPath() / "test.yaml";
    std::filesystem::create_directories(out.parent_path());
    std::ofstream outFileStream(out);
    outFileStream << output;
    outFileStream.close();
    REQUIRE(!output.empty());
}