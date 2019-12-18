// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"


namespace AppInstaller::Repository::Microsoft::Schema
{
    // The common interface used to interact with all schema versions of the index.
    struct ISQLiteIndex
    {
        virtual ~ISQLiteIndex() = default;
    };


    // Common base class used by all schema versions.
    struct SQLiteIndexBase : public ISQLiteIndex
    {
    };
}
