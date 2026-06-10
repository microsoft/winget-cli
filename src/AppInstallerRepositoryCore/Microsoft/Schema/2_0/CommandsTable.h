// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/2_0/OneToManyTableWithMap.h"


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct CommandsTableInfo
        {
            inline static constexpr std::string_view TableName() { return "commands2"sv; }
            inline static constexpr std::string_view ValueName() { return "command"sv; }
        };
    }

    using CommandsTable = OneToManyTableWithMap<details::CommandsTableInfo>;
}
