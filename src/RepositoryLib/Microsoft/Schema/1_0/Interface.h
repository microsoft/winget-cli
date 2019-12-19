// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public SQLiteIndexBase
    {
        Schema::Version GetVersion() const override;
    };
}
