// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"

#include <string_view>

namespace AppInstaller::Repository::Microsoft
{
    // Holds the connection to the database, as well as the appropriate functionality to interface with it.
    struct MSIndex
    {
        static MSIndex CreateNew(std)

    private:
        SQLite::Connection _dbconn;
    };
}
