// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// WinGetRpcTestHelper - a minimal Win32 console binary used by the RPC security
// E2E tests (RpcSecurityTests.cs).
//
// MODES (--mode <name> [options])
//   pipe-access --pipe-name <name>
//       Opens \\.\pipe\<name> for GENERIC_WRITE at current integrity.
//       Expects ERROR_ACCESS_DENIED from the high-integrity SACL on the pipe SD.
//
//   event-signal --event-name <name>
//       Opens <name> with EVENT_MODIFY_STATE at current integrity.
//       Expects ERROR_ACCESS_DENIED from the high-integrity SACL on the event SD.
//
//   event-open --event-name <name>
//       Same as event-signal; separate mode for test clarity.
//
//   rpc-connect
//       Calls WinGetServerManualActivation_CreateInstance for a simple options class.
//       Uses the production client code path, so the ncalrpc binding is configured with
//       mutual authentication and a server security descriptor requiring the same user
//       at high integrity.
//       Exit codes:
//         0 = the server was accepted and the object was created
//         0x80070005 (E_ACCESSDENIED) = the ALPC connect access check rejected the
//           server, or the server's interface security descriptor rejected this client

#include <windows.h>
#include <objbase.h>
#include <string>
#include <vector>

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

// Convert wide string to narrow (ACP).
static std::string WideToNarrow(const wchar_t* w)
{
    if (!w) return {};
    int len = WideCharToMultiByte(CP_ACP, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_ACP, 0, w, -1, result.data(), len, nullptr, nullptr);
    return result;
}

// Find "--flag" in argv and return the next argument, or nullptr.
static const wchar_t* GetFlag(int argc, wchar_t* argv[], const wchar_t* flag)
{
    for (int i = 1; i < argc - 1; ++i)
        if (_wcsicmp(argv[i], flag) == 0) return argv[i + 1];
    return nullptr;
}

// ---------------------------------------------------------------------------
// pipe-access  (0=denied/pass  1=opened/fail  2=unexpected OS error)
// ---------------------------------------------------------------------------

static int TestPipeAccess(const char* pipeName)
{
    std::string path = std::string("\\\\.\\pipe\\") + pipeName;
    HANDLE hPipe = CreateFileA(path.c_str(), GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipe);
        return 1; // security broken: medium-IL wrote to high-integrity pipe
    }
    return (GetLastError() == ERROR_ACCESS_DENIED) ? 0 : 2;
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

// WINGET_INPROC_COM_CLSID_FindPackagesOptions — a simple options object the server
// can create with no side-effects; used to get past CallCreateInstance so the
// post-call integrity check can run.
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

    if (_wcsicmp(mode, L"pipe-access") == 0)
    {
        const wchar_t* name = GetFlag(argc, argv, L"--pipe-name");
        if (!name) return 3;
        return TestPipeAccess(WideToNarrow(name).c_str());
    }
    else if (_wcsicmp(mode, L"event-signal") == 0 || _wcsicmp(mode, L"event-open") == 0)
    {
        const wchar_t* name = GetFlag(argc, argv, L"--event-name");
        if (!name) return 3;
        return TestEventWriteAccess(name);
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
