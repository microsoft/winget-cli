// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winsqlite/winsqlite3.h>

// Adapted from the file sqliteicu.h to use the built-in Windows SQLite and ICU binaries.

extern "C"
{
    int sqlite3IcuInit(sqlite3* db);
}
