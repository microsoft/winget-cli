// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Windows.h>

extern "C" HRESULT WinGetServerManualActivation_CreateInstance(REFCLSID rclsid, REFIID riid, UINT32 flags, void** out);

extern "C" HRESULT WinGetServerManualActivation_Terminate();
