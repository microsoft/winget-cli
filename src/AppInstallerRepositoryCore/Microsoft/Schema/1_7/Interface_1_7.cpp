// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_7/Interface.h"
#include "Microsoft/Schema/1_6/UpgradeCodeTable.h"
#include "Microsoft/Schema/1_6/SearchResultsTable.h"
#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "Microsoft/Schema/1_0/VersionTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_7
{
    Interface::Interface(Utility::NormalizationVersion normVersion) : V1_6::Interface(normVersion)
    {
    }

    Schema::Version Interface::GetVersion() const
    {
        return { 1, 7 };
    }

    V1_0::OneToManyTableSchema Interface::GetOneToManyTableSchema() const
    {
        return V1_0::OneToManyTableSchema::Version_1_7;
    }
}
