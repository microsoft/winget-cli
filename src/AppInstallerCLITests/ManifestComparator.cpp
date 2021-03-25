// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Workflows/ManifestComparator.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility;

using Manifest = ::AppInstaller::Manifest::Manifest;

ManifestInstaller& AddInstaller(Manifest& manifest, Architecture architecture, InstallerTypeEnum installerType, ScopeEnum scope = ScopeEnum::Unknown, std::string minOSversion = {})
{
    ManifestInstaller toAdd;
    toAdd.Arch = architecture;
    toAdd.InstallerType = installerType;
    toAdd.Scope = scope;
    toAdd.MinOSVersion = minOSversion;

    manifest.Installers.emplace_back(std::move(toAdd));

    return manifest.Installers.back();
}

TEST_CASE("ManifestComparator_OSFilter", "[manifest_comparator]")
{
}
