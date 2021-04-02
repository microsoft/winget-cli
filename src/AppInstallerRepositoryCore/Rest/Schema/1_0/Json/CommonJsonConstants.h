// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"

namespace AppInstaller::Repository::Rest::Schema::V1_0::Json
{
    // General API response constants
    constexpr std::string_view Data = "Data"sv;
    constexpr std::string_view ContinuationToken = "ContinuationToken"sv; 

    // General API Header constant
    constexpr std::string_view ContractVersion = "Version"sv;

    // General endpoint constants
    constexpr std::string_view InformationGetEndpoint = "/information"sv;
}
