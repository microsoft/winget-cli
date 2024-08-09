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
    /// Exports the manifest and single manifest installer to a yaml file.
    /// </summary>
    /// <param name="manifest">Manifest object.</param>
    /// <param name="installer">Manifest installer object.</param>
    /// <param name="out">Path of the yaml file.</param>
    void OutputYamlFile(const Manifest& manifest, const ManifestInstaller& installer, const std::filesystem::path& out);
}