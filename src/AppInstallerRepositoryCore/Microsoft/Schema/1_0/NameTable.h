// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/1_0/OneToOneTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct NameTableInfo
        {
            inline static constexpr std::string_view TableName() { return "names"sv; }
            inline static constexpr std::string_view ValueName() { return "name"sv; }
        };
    }

    // The table for Name.
    // TODO: Currently only indexing name from default locale, might need to be OneToMany table
    using NameTable = OneToOneTable<details::NameTableInfo>;
}
