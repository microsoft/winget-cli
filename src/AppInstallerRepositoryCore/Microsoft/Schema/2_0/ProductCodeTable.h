// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/2_0/SystemReferenceStringTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct ProductCodeTableInfo
        {
            inline static constexpr std::string_view TableName() { return "productcodes2"sv; }
            inline static constexpr std::string_view ValueName() { return "productcode"sv; }
        };
    }

    using ProductCodeTable = SystemReferenceStringTable<details::ProductCodeTableInfo>;
}
