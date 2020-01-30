// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/1_0/OneToOneTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct VersionTableInfo
        {
            inline static constexpr std::string_view TableName() { return "versions"sv; }
            inline static constexpr std::string_view ValueName() { return "version"sv; }
        };
    }

    // The table for Version.
    using VersionTable = OneToOneTable<details::VersionTableInfo>;
}
