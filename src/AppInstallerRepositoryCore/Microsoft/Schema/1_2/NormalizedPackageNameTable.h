// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/1_0/OneToManyTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_2
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct NormalizedPackageNameTableInfo
        {
            inline static constexpr std::string_view TableName() { return "norm_names"sv; }
            inline static constexpr std::string_view ValueName() { return "norm_name"sv; }
        };
    }

    // The table for NormalizedPackageName.
    using NormalizedPackageNameTable = V1_0::OneToManyTable<details::NormalizedPackageNameTableInfo>;
}
