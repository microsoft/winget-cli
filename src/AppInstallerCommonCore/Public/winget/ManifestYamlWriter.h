// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Manifest.h>
#include <winget/Yaml.h>

namespace AppInstaller::Manifest::YamlWriter
{
    // Converts a Manifest object to a Yaml node for writing to a yaml file.
    struct ManifestYamlDepopulator
    {
        static std::string DepopulateManifest(const Manifest& manifest);

    private:
        std::ofstream m_outputStream;
    };
}