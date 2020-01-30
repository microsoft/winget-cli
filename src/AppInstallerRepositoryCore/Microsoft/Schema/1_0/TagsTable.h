// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/1_0/OneToManyTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct TagsTableInfo
        {
            inline static constexpr std::string_view TableName() { return "tags"sv; }
            inline static constexpr std::string_view ValueName() { return "tag"sv; }
        };
    }

    // The table for Tags.
    using TagsTable = OneToManyTable<details::TagsTableInfo>;
}
