// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/2_0/OneToManyTableWithMap.h"


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct TagsTableInfo
        {
            inline static constexpr std::string_view TableName() { return "tags2"sv; }
            inline static constexpr std::string_view ValueName() { return "tag"sv; }
        };
    }

    using TagsTable = OneToManyTableWithMap<details::TagsTableInfo>;
}
