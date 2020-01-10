// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/1_0/OneToManyTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct ProtocolsTableInfo
        {
            inline static std::string_view TableName() { return "protocols"sv; }
            inline static std::string_view ValueName() { return "protocol"sv; }
        };
    }

    // The table for Protocols.
    using ProtocolsTable = OneToManyTable<details::ProtocolsTableInfo>;
}
