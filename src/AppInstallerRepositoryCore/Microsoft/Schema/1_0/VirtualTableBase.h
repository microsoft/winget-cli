// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    // Since we have multiple manifest columns pointing to same table now(i.e. versions table), and previous assumption
    // is manifest table column always has same name as referenced table column (i.e. manifest.version and versions.version),
    // we need to differentiate manifest column name and referenced table column name(i.e. manifest.arp_min_version and versions.version)
    // An optional ManifestColumnName() is added to virtual table for the above purpose, and can be used in the future if needed.
    // To let the template codes better determine virtual tables, the following struct is created.

    // Struct used as the base for virtual tables.
    // Future virtual tables reusing an existing table should derive from this and implement
    //    static std::string_view ManifestColumnName();
    // in addition to regular table info methods.
    struct VirtualTableBase
    {
    };
}
