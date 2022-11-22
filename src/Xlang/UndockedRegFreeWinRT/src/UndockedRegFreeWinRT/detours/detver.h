//////////////////////////////////////////////////////////////////////////////
//
//  Common version parameters.
//
//  Microsoft Research Detours Package, Version 4.0.1
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//

#define _USING_V110_SDK71_ 1
#include "winver.h"
#if 0
#include <windows.h>
#include <detours.h>
#else
#ifndef DETOURS_STRINGIFY
#define DETOURS_STRINGIFY(x)    DETOURS_STRINGIFY_(x)
#define DETOURS_STRINGIFY_(x)    #x
#endif

#define VER_FILEFLAGSMASK   0x3fL
#define VER_FILEFLAGS       0x0L
#define VER_FILEOS          0x00040004L
#define VER_FILETYPE        0x00000002L
#define VER_FILESUBTYPE     0x00000000L
#endif
#define VER_DETOURS_BITS    DETOUR_STRINGIFY(DETOURS_BITS)
