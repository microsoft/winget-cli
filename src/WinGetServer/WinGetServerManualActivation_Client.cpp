// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "WinGetServer.h"
#include "WinGetServerManualActivation_Client.h"
#include "appmodel.h"
#include "Utils.h"

#include <wil/com.h>
#include <wil/result.h>
#include <wil/safecast.h>

#include <memory>
#include <mutex>
#include <string>
#include <shtypes.h>
#include <filesystem>
#include <shlobj_core.h>
#define SECURITY_WIN32
#include <Security.h>

#ifdef USE_PROD_WINGET_SERVER
const std::wstring_view s_ServerPackageFamilyName = L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe";
const std::wstring_view s_ServerFileName = L"WindowsPackageManagerServer.exe";
#else
const std::wstring_view s_LocalAppDataRelativeServerExePath = L"Microsoft\\WindowsApps\\WinGetDevCLI_8wekyb3d8bbwe\\WindowsPackageManagerServerDev.exe";
const std::wstring_view s_ServerPackageFamilyName = L"WinGetDevCLI_8wekyb3d8bbwe";
const std::wstring_view s_ServerFileName = L"WinGetServer\\WindowsPackageManagerServer.exe";
#endif

_Must_inspect_result_
_Ret_maybenull_ _Post_writable_byte_size_(size)
void* __RPC_USER MIDL_user_allocate(_In_ size_t size)
{
    return malloc(size);
}

void __RPC_USER MIDL_user_free(_Pre_maybenull_ _Post_invalid_ void* ptr)
{
    if (ptr)
    {
        free(ptr);
    }
}

struct FreeWithRpcStringFree { void operator()(RPC_CSTR* in) { RpcStringFreeA(in); } };
using UniqueRpcString = std::unique_ptr<RPC_CSTR, FreeWithRpcStringFree>;

struct DeleteWithMidlFree { void operator()(void* m) { MIDL_user_free(m); } };
using UniqueMidl = std::unique_ptr<BYTE, DeleteWithMidlFree>;

// Validates that the server process token belongs to the current user and is running
// at high mandatory integrity.  Throws HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) on
// any violation.
static void VerifyServerProcessToken(HANDLE hToken)
{
    // --- Integrity level check ---
    DWORD ilSize = 0;
    GetTokenInformation(hToken, TokenIntegrityLevel, nullptr, 0, &ilSize);
    auto ilBuf = std::make_unique<BYTE[]>(ilSize);
    THROW_LAST_ERROR_IF(!GetTokenInformation(hToken, TokenIntegrityLevel, ilBuf.get(), ilSize, &ilSize));
    auto* pLabel = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(ilBuf.get());
    DWORD rid = *GetSidSubAuthority(pLabel->Label.Sid, *GetSidSubAuthorityCount(pLabel->Label.Sid) - 1);
    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), rid < SECURITY_MANDATORY_HIGH_RID);

    // --- User SID check ---
    DWORD userSize = 0;
    GetTokenInformation(hToken, TokenUser, nullptr, 0, &userSize);
    auto userBuf = std::make_unique<BYTE[]>(userSize);
    THROW_LAST_ERROR_IF(!GetTokenInformation(hToken, TokenUser, userBuf.get(), userSize, &userSize));
    auto* pUser = reinterpret_cast<TOKEN_USER*>(userBuf.get());
    THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), !IsCurrentUserSid(pUser->User.Sid));
}

// Opens the named pipe as a client, retrieves the server process PID, opens the
// server process token, and runs VerifyServerProcessToken against it.
// Throws HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) on any violation.
static void VerifyPipeServerIntegrity(const std::string& pipePath)
{
    wil::unique_hfile pipeHandle{ CreateFileA(
        pipePath.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr) };
    THROW_LAST_ERROR_IF(!pipeHandle);

    ULONG serverPid = 0;
    THROW_LAST_ERROR_IF(!GetNamedPipeServerProcessId(pipeHandle.get(), &serverPid));

    wil::unique_handle hProcess{ OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, serverPid) };
    THROW_LAST_ERROR_IF(!hProcess);

    wil::unique_handle hToken;
    THROW_LAST_ERROR_IF(!OpenProcessToken(hProcess.get(), TOKEN_QUERY, hToken.put()));

    VerifyServerProcessToken(hToken.get());
}

std::string GetServerPipeName()
{
    return "\\pipe\\WinGetServerManualActivation_" + GetUserSID();
}

void InitializeRpcBinding()
{
    std::string protocol = "ncacn_np";
    std::string endpoint = GetServerPipeName();

    unsigned char* binding = nullptr;
    UniqueRpcString bindingPtr;

    RPC_STATUS status = RpcStringBindingComposeA(nullptr, GetUCharString(protocol), nullptr, GetUCharString(endpoint), nullptr, &binding);
    THROW_HR_IF(HRESULT_FROM_WIN32(status), status != RPC_S_OK);
    bindingPtr.reset(&binding);

    status = RpcBindingFromStringBindingA(binding, &WinGetServerManualActivation_IfHandle);
    THROW_HR_IF(HRESULT_FROM_WIN32(status), status != RPC_S_OK);

    // Provide the current user's UPN as the SPN so Negotiate can select Kerberos and
    // the server identity is pinned to the expected principal (same user as the client).
    // On non-domain machines NameUserPrincipal is not supported; in that case we omit
    // the principal name and fall back to NTLM. The post-call process token check still
    // validates identity in that scenario.
    std::unique_ptr<char[]> upnBuf;
    ULONG upnLength = 0;
    GetUserNameExA(NameUserPrincipal, nullptr, &upnLength);
    if (upnLength > 0)
    {
        upnBuf = std::make_unique<char[]>(upnLength);
        if (!GetUserNameExA(NameUserPrincipal, upnBuf.get(), &upnLength))
        {
            upnBuf.reset();
        }
    }

    RPC_SECURITY_QOS qos{};
    qos.Version = RPC_C_SECURITY_QOS_VERSION;
    qos.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    qos.IdentityTracking = RPC_C_QOS_IDENTITY_STATIC;
    qos.ImpersonationType = RPC_C_IMP_LEVEL_IDENTIFY;

    status = RpcBindingSetAuthInfoExA(
        WinGetServerManualActivation_IfHandle,
        upnBuf ? GetUCharString(upnBuf.get()) : nullptr,  // UPN when available; nullptr on non-domain machines
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_AUTHN_GSS_NEGOTIATE,
        nullptr,                       // use default credentials
        RPC_C_AUTHZ_NONE,
        &qos);
    THROW_HR_IF(HRESULT_FROM_WIN32(status), status != RPC_S_OK);
}

struct ServerProcessLauncher
{
    ServerProcessLauncher()
    {
        try
        {
            m_serverExePath = GetPackageLocation(s_ServerPackageFamilyName, s_ServerFileName) / s_ServerFileName;

#ifndef USE_PROD_WINGET_SERVER
            // The feature that allows directly launching a packaged process as long as it has a matching alias
            // requires a failure to trigger, and the dev package ACL does not force this to happen. Attempting
            // to use the other code path results in an unpackaged server, causing other issues.
            // We run the product code above to ensure that it is functioning properly, but then replace it with
            // the path of the alias.
            m_serverExePath = GetKnownFolderPath(FOLDERID_LocalAppData) / s_LocalAppDataRelativeServerExePath;
#endif
        }
        catch (wil::ResultException& re)
        {
            m_hr = re.GetErrorCode();
        }
    }

    HRESULT LaunchWinGetServerWithManualActivation()
    {
        RETURN_IF_FAILED(m_hr);

        std::wstring commandLineInput = L"\"" + std::wstring{ m_serverExePath } + L"\" --manualActivation";

        STARTUPINFO info = { sizeof(info) };
        wil::unique_process_information process;

        RETURN_LAST_ERROR_IF(!CreateProcessW(NULL, &commandLineInput[0], NULL, NULL, FALSE, 0, NULL, NULL, &info, &process));

        // Wait for manual reset event from server before proceeding with COM activation.
        wil::unique_event manualResetEvent = CreateOrOpenServerStartEvent();
        manualResetEvent.wait(10000);

        return S_OK;
    }

private:
    std::filesystem::path GetPackageLocation(std::wstring_view packageFamilyName, std::wstring_view fileName)
    {
        std::wstring pfn{ packageFamilyName };
        UINT32 count = 0;
        std::unique_ptr<PWSTR[]> fullNames;
        UINT32 bufferLength = 0;
        std::unique_ptr<WCHAR[]> buffer;
        std::unique_ptr<UINT32[]> properties;

        LONG result = FindPackagesByPackageFamily(pfn.c_str(), PACKAGE_FILTER_HEAD, &count, nullptr, &bufferLength, nullptr, nullptr);
        THROW_WIN32_IF(result, result != ERROR_INSUFFICIENT_BUFFER);

        for (size_t i = 0; i < 10 && result == ERROR_INSUFFICIENT_BUFFER; ++i)
        {
            fullNames = std::make_unique<PWSTR[]>(count);
            buffer = std::make_unique<WCHAR[]>(bufferLength);
            properties = std::make_unique<UINT32[]>(count);

            result = FindPackagesByPackageFamily(pfn.c_str(), PACKAGE_FILTER_HEAD, &count, fullNames.get(), &bufferLength, buffer.get(), properties.get());
        }

        THROW_IF_WIN32_ERROR(result);

        for (UINT32 i = 0; i < count; ++i)
        {
            // Includes null terminator
            UINT32 pathLength = 0;
            result = GetPackagePathByFullName(fullNames[i], &pathLength, nullptr);
            if (result != ERROR_INSUFFICIENT_BUFFER)
            {
                continue;
            }

            std::wstring packagePath;
            packagePath.resize(static_cast<size_t>(pathLength));

            if (FAILED_WIN32(GetPackagePathByFullName(fullNames[i], &pathLength, &packagePath[0])))
            {
                continue;
            }
            packagePath.resize(static_cast<size_t>(pathLength - 1), L'\0');

            std::filesystem::path resultPath = std::move(packagePath);
            std::filesystem::path exePath = resultPath / fileName;

            if (GetFileAttributesW(exePath.c_str()) != INVALID_FILE_ATTRIBUTES)
            {
                return resultPath;
            }
        }

        THROW_WIN32(ERROR_PACKAGE_NOT_REGISTERED_FOR_USER);
    }

    std::filesystem::path GetKnownFolderPath(const KNOWNFOLDERID& id)
    {
        wil::unique_cotaskmem_string knownFolder = nullptr;
        THROW_IF_FAILED(SHGetKnownFolderPath(id, KF_FLAG_NO_ALIAS | KF_FLAG_DONT_VERIFY | KF_FLAG_NO_PACKAGE_REDIRECTION, NULL, &knownFolder));
        return knownFolder.get();
    }

    std::filesystem::path m_serverExePath;
    HRESULT m_hr = S_OK;
};

HRESULT CallCreateInstance(REFCLSID rclsid, REFIID riid, UINT32 flags, UINT32* bufferByteCount, BYTE** buffer)
{
    RpcTryExcept
    {
        RETURN_IF_FAILED(CreateInstance(rclsid, riid, flags, bufferByteCount, buffer));
    }
    RpcExcept(1)
    {
        return HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return S_OK;
}

HRESULT CreateComInstance(REFCLSID rclsid, REFIID riid, UINT32 flags, void** out)
{
    UINT32 bufferByteCount = 0;
    BYTE* buffer = nullptr;
    UniqueMidl bufferPtr;

    RETURN_IF_FAILED(CallCreateInstance(rclsid, riid, flags, &bufferByteCount, &buffer));
    bufferPtr.reset(buffer);

    // The RPC call reached the server; verify its integrity and user before unmarshaling
    // any server-provided data. The check runs here rather than during binding setup
    // because the server process may not exist until after the first successful call.
    std::string pipePath = GetServerPipeName();
    try
    {
        VerifyPipeServerIntegrity(pipePath);
    }
    CATCH_RETURN();

    wil::com_ptr<IStream> stream;
    RETURN_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream));
    RETURN_IF_FAILED(stream->Write(buffer, bufferByteCount, nullptr));
    RETURN_IF_FAILED(stream->Seek({}, STREAM_SEEK_SET, nullptr));

    wil::com_ptr<IUnknown> output;
    RETURN_IF_FAILED(CoUnmarshalInterface(stream.get(), riid, reinterpret_cast<void**>(&output)));
    *out = output.detach();
    return S_OK;
}

extern "C" HRESULT WinGetServerManualActivation_CreateInstance(REFCLSID rclsid, REFIID riid, UINT32 flags, void** out)
{
    RETURN_HR_IF_NULL(E_POINTER, out);

#ifndef AICLI_DISABLE_TEST_HOOKS
    bool noServerLaunch = (flags & WinGetServerManualActivation_TestHookFlag_NoServerLaunch) != 0;
    flags &= ~WinGetServerManualActivation_TestHookFlag_NoServerLaunch;
#endif

    static std::once_flag rpcBindingOnce;
    try
    {
        std::call_once(rpcBindingOnce, InitializeRpcBinding);
    }
    CATCH_RETURN();

    HRESULT result = CreateComInstance(rclsid, riid, flags, out);
    if (FAILED(result))
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (noServerLaunch)
        {
            return result;
        }
#endif

        ServerProcessLauncher launcher;

        for (int i = 0; i < 3; i++)
        {
            result = launcher.LaunchWinGetServerWithManualActivation();
            if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || result == HRESULT_FROM_WIN32(ERROR_PACKAGE_NOT_REGISTERED_FOR_USER))
            {
                break;
            }

            result = CreateComInstance(rclsid, riid, flags, out);
            if (SUCCEEDED(result))
            {
                break;
            }

            Sleep(200);
        }
    }

    return result;
}

extern "C" HRESULT WinGetServerManualActivation_Terminate()
{
    RpcBindingFree(&WinGetServerManualActivation_IfHandle);
    return S_OK;
}
