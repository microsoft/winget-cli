// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

void UnloadAndCheckForLeaks()
{
    CoFreeUnusedLibrariesEx(0, 0);
}
