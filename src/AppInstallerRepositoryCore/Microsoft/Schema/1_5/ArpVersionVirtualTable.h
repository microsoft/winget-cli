// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteStatementBuilder.h>
#include "Microsoft/Schema/1_0/VersionTable.h"
#include "Microsoft/Schema/1_0/VirtualTableBase.h"
#include <string_view>

using namespace std::string_view_literals;

namespace AppInstaller::Repository::Microsoft::Schema::V1_5
{
    // A virtual table used to add Arp min version to ManifestTable, the values are stored in VersionTable.
    struct ArpMinVersionVirtualTable : public V1_0::VirtualTableBase
    {
        // The id type
        using id_t = V1_0::VersionTable::id_t;

        // The value type
        using value_t = V1_0::VersionTable::value_t;

        // The name of the table.
        static constexpr std::string_view TableName()
        {
            return V1_0::VersionTable::TableName();
        }

        // The value name of the column.
        static constexpr std::string_view ValueName()
        {
            return V1_0::VersionTable::ValueName();
        }

        // The value name of the manifest table column.
        static constexpr std::string_view ManifestColumnName()
        {
            return "arp_min_version"sv;
        }
    };

    // A virtual table used to add Arp max version to ManifestTable, the values are stored in VersionTable.
    struct ArpMaxVersionVirtualTable : public V1_0::VirtualTableBase
    {
        // The id type
        using id_t = V1_0::VersionTable::id_t;

        // The value type
        using value_t = V1_0::VersionTable::value_t;

        // The name of the table.
        static constexpr std::string_view TableName()
        {
            return V1_0::VersionTable::TableName();
        }

        // The value name of the column.
        static constexpr std::string_view ValueName()
        {
            return V1_0::VersionTable::ValueName();
        }

        // The value name of the manifest table column.
        static constexpr std::string_view ManifestColumnName()
        {
            return "arp_max_version"sv;
        }
    };
}
