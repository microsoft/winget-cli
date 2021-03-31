// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"

namespace AppInstaller::Repository::Rest::Schema::Json
{
    // General API response constants
    constexpr std::string_view Data = "Data"sv;
    constexpr std::string_view ContractVersion = "Version"sv;
    constexpr std::string_view ContinuationToken = "ContinuationToken"sv; 

    // General endpoint constants
    constexpr std::string_view InformationGetEndpoint = "/information"sv;

    // Winget supported contract versions
    const Utility::Version Version_0_2_0 { "0.2.0" }; // TODO: Remove once winget schema is finalized.
    const Utility::Version Version_1_0_0 { "1.0.0" };
}
