// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/1_0/OneToManyTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_1
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct PackageFamilyNameTableInfo
        {
            inline static constexpr std::string_view TableName() { return "pfns"sv; }
            inline static constexpr std::string_view ValueName() { return "pfn"sv; }
        };
    }

    // The table for PackageFamilyName.
    using PackageFamilyNameTable = V1_0::OneToManyTable<details::PackageFamilyNameTableInfo>;
}
