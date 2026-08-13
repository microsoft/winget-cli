// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// WinGetRpcTestHelper - a minimal Win32 console binary used by the RPC security
// E2E tests (RpcSecurityTests.cs).
//
// MODES (--mode <name> [options])
//   event-signal --event-name <name>
//       Opens <name> with EVENT_MODIFY_STATE at current integrity.
//       Expects ERROR_ACCESS_DENIED from the high-integrity SACL on the event SD.
//
//   mutex-open --mutex-name <name>
//       Opens <name> with SYNCHRONIZE at current integrity, which is the access a process
//       would need to acquire the mutex and hold it to keep the server from starting.
//       Expects ERROR_ACCESS_DENIED from the high-integrity SACL on the mutex SD.
//
//   rpc-connect
//       Calls WinGetServerManualActivation_CreateInstance for a simple options class.
//       Uses the production client code path, so the ncalrpc binding is configured with
//       mutual authentication and a server security descriptor requiring the same user
//       at high integrity.
//       Exit codes:
//         0 = the server was accepted and the object was created
//         0x80070005 (E_ACCESSDENIED) = the connection was rejected, either because the
//           server failed the client's server security descriptor check, or because this
//           process failed the server's endpoint or interface security descriptor

#include <windows.h>
#include <objbase.h>

// RPC headers and generated client stub.
// WinGetServer_c.c is compiled as a separate C translation unit in the project file.
#include "WinGetServer.h"

// Production client: WinGetServerManualActivation_CreateInstance and friends.
// WinGetServerManualActivation_Client.cpp (compiled as a separate TU) also
// provides MIDL_user_allocate / MIDL_user_free.
#include "WinGetServerManualActivation_Client.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Find "--flag" in argv and return the next argument, or nullptr.
static const wchar_t* GetFlag(int argc, wchar_t* argv[], const wchar_t* flag)
{
    for (int i = 1; i < argc - 1; ++i)
        if (_wcsicmp(argv[i], flag) == 0) return argv[i + 1];
    return nullptr;
}

// ---------------------------------------------------------------------------
// event-signal / event-open  (0=denied/pass  1=opened/fail  2=unexpected OS error)
// ---------------------------------------------------------------------------

static int TestEventWriteAccess(const wchar_t* eventName)
{
    HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, eventName);
    if (hEvent)
    {
        CloseHandle(hEvent);
        return 1; // security broken
    }
    return (GetLastError() == ERROR_ACCESS_DENIED) ? 0 : 2;
}

// ---------------------------------------------------------------------------
// mutex-open  (0=denied/pass  1=opened/fail  2=unexpected OS error)
// ---------------------------------------------------------------------------

static int TestMutexAcquireAccess(const wchar_t* mutexName)
{
    HANDLE hMutex = OpenMutexW(SYNCHRONIZE, FALSE, mutexName);
    if (hMutex)
    {
        CloseHandle(hMutex);
        return 1; // security broken
    }
    return (GetLastError() == ERROR_ACCESS_DENIED) ? 0 : 2;
}

// WINGET_INPROC_COM_CLSID_FindPackagesOptions — a simple options object the server
// can create with no side-effects, so the call exercises the security configuration
// rather than failing early on an unavailable class.
static const CLSID s_clsidFindPackagesOptions = { 0x1bd8ff3a,0xec50,0x4f69,{0xae,0xee,0xdf,0x4c,0x9d,0x3b,0xaa,0x96} };

// ---------------------------------------------------------------------------
// rpc-connect  (uses production code; see mode comment above for exit codes)
// ---------------------------------------------------------------------------

static int TestRpcConnectViaProductCode()
{
    void* out = nullptr;
    HRESULT hr = WinGetServerManualActivation_CreateInstance(
        s_clsidFindPackagesOptions, IID_IUnknown, WinGetServerManualActivation_TestHookFlag_NoServerLaunch, &out);
    if (out)
    {
        reinterpret_cast<IUnknown*>(out)->Release();
    }
    return hr;
}

// ---------------------------------------------------------------------------
// Entry point
// (2=--mode missing  3=mode-specific argument missing  4=unknown mode)
// ---------------------------------------------------------------------------

int wmain(int argc, wchar_t* argv[])
{
    const wchar_t* mode = GetFlag(argc, argv, L"--mode");
    if (!mode) return 2;

    if (_wcsicmp(mode, L"event-signal") == 0)
    {
        const wchar_t* name = GetFlag(argc, argv, L"--event-name");
        if (!name) return 3;
        return TestEventWriteAccess(name);
    }
    else if (_wcsicmp(mode, L"mutex-open") == 0)
    {
        const wchar_t* name = GetFlag(argc, argv, L"--mutex-name");
        if (!name) return 3;
        return TestMutexAcquireAccess(name);
    }
    else if (_wcsicmp(mode, L"rpc-connect") == 0)
    {
        // COM must be initialized before WinGetServerManualActivation_CreateInstance
        // can unmarshal the returned COM object.
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        return TestRpcConnectViaProductCode();
    }

    return 4; // unknown mode
}
