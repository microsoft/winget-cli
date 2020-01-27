// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/1_0/OneToManyTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct CommandsTableInfo
        {
            inline static constexpr std::string_view TableName() { return "commands"sv; }
            inline static constexpr std::string_view ValueName() { return "command"sv; }
        };
    }

    // The table for Commands.
    using CommandsTable = OneToManyTable<details::CommandsTableInfo>;
}
