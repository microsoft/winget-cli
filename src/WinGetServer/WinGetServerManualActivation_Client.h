// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Windows.h>

extern "C" HRESULT WinGetServerManualActivation_CreateInstance(REFCLSID rclsid, REFIID riid, UINT32 flags, void** out);

extern "C" HRESULT WinGetServerManualActivation_Terminate();

#ifndef AICLI_DISABLE_TEST_HOOKS
// When passed in the `flags` argument, suppresses the ServerProcessLauncher fallback
// path so tests can rely on a pre-started server without accidentally launching another.
// Stripped from flags before the value is forwarded to the server over RPC.
const UINT32 WinGetServerManualActivation_TestHookFlag_NoServerLaunch = 0x80000000;
#endif
