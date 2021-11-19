// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WindowsPackageManager.h>

int wmain(int argc, wchar_t const** argv)
{
    return WindowsPackageManagerCLIMain(argc, argv);
}
