// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#ifdef CLICOREDLLBUILD
    #define CLICOREAPIEXPORT __declspec(dllexport)
#else
    #define CLICOREAPIEXPORT __declspec(dllimport)
#endif

#define CLICORE_ERROR_FACILITY 0x8A150000

#define CLICORE_ERROR_INVALID_CL_ARGUMENTS 0x8A150001
#define CLICORE_ERROR_INTERNAL_ERROR       0x8A150002

extern "C"
{
    CLICOREAPIEXPORT int CLICoreMain(int argc, wchar_t const** argv);
}
