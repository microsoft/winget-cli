// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Manifest.h>
#include <winget/Yaml.h>

namespace AppInstaller::Manifest::YamlWriter
{
    /// <summary>
    /// Converts the manifest and a single installer to a yaml string.
    /// </summary>
    /// <param name="manifest">Manifest object.</param>
    /// <param name="installer">Manifest installer object.</param>
    /// <returns>Yaml string.</returns>
    std::string ManifestToYamlString(const Manifest& manifest, const ManifestInstaller& installer);

    /// <summary>
    /// Exports the manifest yaml string to a file.
    /// </summary>
    /// <param name="content">Manifest yaml string.</param>
    /// <param name="out">Path of the yaml file.</param>
    void OutputYamlFile(const std::string& content, const std::filesystem::path& out);
}