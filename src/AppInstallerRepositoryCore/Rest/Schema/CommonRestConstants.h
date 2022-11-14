// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>

namespace AppInstaller::Repository::Rest::Schema
{
    // Winget supported contract versions
    const Utility::Version Version_1_0_0{ "1.0.0" };
    const Utility::Version Version_1_1_0{ "1.1.0" };
    const Utility::Version Version_1_4_0{ "1.4.0" };

    // General API response constants
    constexpr std::string_view Data = "Data"sv;
    constexpr std::string_view ContinuationToken = "ContinuationToken"sv;

    // General API Header constant
    constexpr std::string_view ContractVersion = "Version"sv;

    // General endpoint constants
    constexpr std::string_view InformationGetEndpoint = "/information"sv;
    constexpr std::string_view ManifestSearchPostEndpoint = "/manifestSearch"sv;
    constexpr std::string_view ManifestByVersionAndChannelGetEndpoint = "/packageManifests/"sv;
}
