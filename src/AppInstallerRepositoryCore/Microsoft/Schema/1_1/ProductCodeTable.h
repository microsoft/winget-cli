// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/1_0/OneToManyTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_1
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct ProductCodeTableInfo
        {
            inline static constexpr std::string_view TableName() { return "productcodes"sv; }
            inline static constexpr std::string_view ValueName() { return "productcode"sv; }
        };
    }

    // The table for ProductCode.
    using ProductCodeTable = V1_0::OneToManyTable<details::ProductCodeTableInfo>;
}
