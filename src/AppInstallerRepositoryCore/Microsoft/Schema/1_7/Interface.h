// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/1_6/Interface.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_7
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public V1_6::Interface
    {
        Interface(Utility::NormalizationVersion normVersion = Utility::NormalizationVersion::Initial);

        // Version 1.0
        Schema::Version GetVersion() const override;

    protected:
        V1_0::OneToManyTableSchema GetOneToManyTableSchema() const override;
    };
}
