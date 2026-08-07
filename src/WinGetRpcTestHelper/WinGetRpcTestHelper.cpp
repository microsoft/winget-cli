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
//   rpc-connect --endpoint <name>
//       Opens \\.\pipe\<name>, obtains the server PID via GetNamedPipeServerProcessId,
//       and verifies the server process is at high mandatory integrity.  If the server
//       is below high integrity the helper exits with ERROR_ACCESS_DENIED (5) without
//       making an RPC call.  If the integrity check passes it performs a full
//       authenticated RPC bind and calls CreateInstance.
//
//   rpc-noauth --endpoint <name>
//       Performs an unauthenticated RPC bind and calls CreateInstance.
//       Expects the server to reject the call.

#include <windows.h>
#include <sddl.h>
#include <string>
#include <vector>

// IID_IUnknown would normally come from objbase.h, but that pulls in full COM headers.
// Define it explicitly to avoid the dependency.
static const GUID s_IID_IUnknown =
    { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

// RPC headers and generated client stub.
// WinGetServer_c.c is compiled as a separate C translation unit in the project file.
#include "WinGetServer.h"

void* __RPC_USER MIDL_user_allocate(size_t size) { return malloc(size); }
void __RPC_USER MIDL_user_free(void* ptr) { free(ptr); }

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Build an RPC string binding for the WinGetServer named-pipe endpoint.
// endpointName is e.g. "WinGetServerManualActivation_S-1-5-..."
// Returns 0 on success.  Returns 2 if RpcStringBindingComposeA fails,
// 3 if RpcBindingFromStringBindingA fails.
static int BuildRpcBinding(const char* endpointName, handle_t& hBinding)
{
    std::string endpoint = std::string("\\pipe\\") + endpointName;

    RPC_CSTR bindingString = nullptr;
    RPC_STATUS status = RpcStringBindingComposeA(
        nullptr,
        reinterpret_cast<RPC_CSTR>(const_cast<char*>("ncacn_np")),
        reinterpret_cast<RPC_CSTR>(const_cast<char*>(".")),
        reinterpret_cast<RPC_CSTR>(const_cast<char*>(endpoint.c_str())),
        nullptr,
        &bindingString);

    if (status != RPC_S_OK) return 2;

    status = RpcBindingFromStringBindingA(bindingString, &hBinding);
    RpcStringFreeA(&bindingString);
    return (status == RPC_S_OK) ? 0 : 3;
}

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
// Server process integrity verification
// ---------------------------------------------------------------------------

// Returns the mandatory integrity level RID for the given process, or 0 on failure.
// High integrity = SECURITY_MANDATORY_HIGH_RID (0x3000).
static DWORD GetProcessIntegrityLevel(DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) return 0;

    HANDLE hToken = nullptr;
    bool opened = OpenProcessToken(hProcess, TOKEN_QUERY, &hToken) != FALSE;
    CloseHandle(hProcess);
    if (!opened) return 0;

    DWORD size = 0;
    GetTokenInformation(hToken, TokenIntegrityLevel, nullptr, 0, &size);

    std::vector<BYTE> buf(size);
    DWORD level = 0;
    if (size > 0 && GetTokenInformation(hToken, TokenIntegrityLevel, buf.data(), size, &size))
    {
        auto* pLabel = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(buf.data());
        DWORD* pRid = GetSidSubAuthority(pLabel->Label.Sid,
            *GetSidSubAuthorityCount(pLabel->Label.Sid) - 1);
        level = *pRid;
    }

    CloseHandle(hToken);
    return level;
}

// Opens \\.\pipe\<pipeName>, retrieves the server process ID, and checks that the
// server process is running at high mandatory integrity.
// Returns 0 if the server is at high integrity (proceed with RPC call).
// Returns ERROR_ACCESS_DENIED if the server is below high integrity (client rejects).
// Returns another OS error code for infrastructure failures (pipe not found, etc.).
static int VerifyServerProcessIntegrity(const char* pipeName)
{
    std::string path = std::string("\\\\.\\pipe\\") + pipeName;
    HANDLE hPipe = CreateFileA(path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

    if (hPipe == INVALID_HANDLE_VALUE) return static_cast<int>(GetLastError());

    ULONG serverPid = 0;
    bool got = GetNamedPipeServerProcessId(hPipe, &serverPid) != FALSE;
    DWORD err = got ? 0 : GetLastError();
    CloseHandle(hPipe);

    if (!got) return static_cast<int>(err);

    DWORD level = GetProcessIntegrityLevel(serverPid);
    return (level >= SECURITY_MANDATORY_HIGH_RID) ? 0 : ERROR_ACCESS_DENIED;
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

// ---------------------------------------------------------------------------
// rpc-connect
//   0                   - integrity check passed AND call reached server -> PASS
//   ERROR_ACCESS_DENIED - server below high integrity (client rejects)   -> PASS
//                         OR medium-IL client blocked by pipe SACL       -> PASS
//   RPC exception code  - transport/auth failure after integrity check passes
//   2  - RpcStringBindingComposeA failed
//   3  - RpcBindingFromStringBindingA failed
//   4  - RpcBindingSetAuthInfoExA failed
//   other OS error      - integrity check infrastructure failure
// ---------------------------------------------------------------------------

static int TestRpcConnect(const char* endpointName)
{
    // Before making any RPC call, verify the server process is running at high
    // mandatory integrity.  This is the client-side check that prevents connecting
    // to a medium-integrity impersonator.  For a medium-IL client connecting to a
    // high-IL server the pipe open itself fails with ERROR_ACCESS_DENIED because
    // the pipe SD carries a high-integrity SACL (Finding 3).
    int integrityCheck = VerifyServerProcessIntegrity(endpointName);
    if (integrityCheck != 0) return integrityCheck;

    handle_t hBinding = nullptr;
    int bindResult = BuildRpcBinding(endpointName, hBinding);
    if (bindResult != 0) return bindResult; // 2 or 3

    // Apply authentication
    RPC_SECURITY_QOS qos{};
    qos.Version           = RPC_C_SECURITY_QOS_VERSION;
    qos.Capabilities      = RPC_C_QOS_CAPABILITIES_DEFAULT;
    qos.IdentityTracking  = RPC_C_QOS_IDENTITY_STATIC;
    qos.ImpersonationType = RPC_C_IMP_LEVEL_IDENTIFY;

    RPC_STATUS status = RpcBindingSetAuthInfoExA(
        hBinding,
        nullptr,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_AUTHN_GSS_NEGOTIATE,
        nullptr,
        RPC_C_AUTHZ_NONE,
        &qos);

    if (status != RPC_S_OK)
    {
        RpcBindingFree(&hBinding);
        return 4;
    }

    WinGetServerManualActivation_IfHandle = hBinding;

    GUID clsidNull{};
    UINT32 cbBuffer = 0;
    BYTE* pBuffer = nullptr;
    bool callCompleted = false;
    RPC_STATUS exceptionCode = RPC_S_OK;

    __try
    {
        CreateInstance(clsidNull, s_IID_IUnknown, 0, &cbBuffer, &pBuffer);
        callCompleted = true;
        if (pBuffer) { MIDL_user_free(pBuffer); }
    }
    __except (exceptionCode = RpcExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
    {
        // RPC transport error: call did not reach the server.
        // exceptionCode holds the RPC_STATUS for the caller to return.
    }

    RpcBindingFree(&hBinding);
    WinGetServerManualActivation_IfHandle = nullptr;

    // 0          = transport reached the server (any app-level HRESULT is fine here)
    // RPC status = transport/auth rejected; the specific code identifies the reason
    return callCompleted ? 0 : static_cast<int>(exceptionCode);
}

// ---------------------------------------------------------------------------
// rpc-noauth  (0=server rejected/pass  1=server accepted/fail
//              RPC exception code=binding setup failed (unexpected)
//              2=RpcStringBindingComposeA failed
//              3=RpcBindingFromStringBindingA failed)
// ---------------------------------------------------------------------------

static int TestRpcNoAuth(const char* endpointName)
{
    handle_t hBinding = nullptr;
    int bindResult = BuildRpcBinding(endpointName, hBinding);
    if (bindResult != 0) return bindResult; // 2 or 3

    // Intentionally skip RpcBindingSetAuthInfoExA - unauthenticated binding.
    WinGetServerManualActivation_IfHandle = hBinding;

    GUID clsidNull{};
    UINT32 cbBuffer = 0;
    BYTE* pBuffer = nullptr;
    bool callCompleted = false;
    RPC_STATUS exceptionCode = RPC_S_OK;

    __try
    {
        CreateInstance(clsidNull, s_IID_IUnknown, 0, &cbBuffer, &pBuffer);
        callCompleted = true;
        if (pBuffer) { MIDL_user_free(pBuffer); }
    }
    __except (exceptionCode = RpcExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {}

    RpcBindingFree(&hBinding);
    WinGetServerManualActivation_IfHandle = nullptr;

    // 1          = server unexpectedly accepted it (security broken)
    // RPC status = call was rejected but via an exception rather than returning normally
    return callCompleted ? 1 : static_cast<int>(exceptionCode);
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
        const wchar_t* ep = GetFlag(argc, argv, L"--endpoint");
        if (!ep) return 3;
        return TestRpcConnect(WideToNarrow(ep).c_str());
    }
    else if (_wcsicmp(mode, L"rpc-noauth") == 0)
    {
        const wchar_t* ep = GetFlag(argc, argv, L"--endpoint");
        if (!ep) return 3;
        return TestRpcNoAuth(WideToNarrow(ep).c_str());
    }

    return 4; // unknown mode
}
