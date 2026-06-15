// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/1_0/OneToManyTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_6
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct UpgradeCodeTableInfo
        {
            inline static constexpr std::string_view TableName() { return "upgradecodes"sv; }
            inline static constexpr std::string_view ValueName() { return "upgradecode"sv; }
        };
    }

    // The table for UpgradeCode.
    using UpgradeCodeTable = V1_0::OneToManyTable<details::UpgradeCodeTableInfo>;
}

