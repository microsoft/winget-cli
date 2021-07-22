// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/1_0/OneToManyTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_3
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct DependenciesTableInfo
        {
            inline static constexpr std::string_view TableName() { return "dependencies"sv; }
            inline static constexpr std::string_view ValueName() { return "id"sv; }
        };
    }

    // The table for Dependencies.
    using DependenciesTable = V1_0::OneToManyTable<details::DependenciesTableInfo>;
}
