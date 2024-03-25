// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "WinGetServer.h"
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

#ifdef USE_PROD_WINGET_SERVER
const std::wstring_view s_ServerExePath = L"Microsoft\\WindowsApps\\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\\WindowsPackageManagerServer.exe";
#else
const std::wstring_view s_ServerExePath = L"Microsoft\\WindowsApps\\WinGetDevCLI_8wekyb3d8bbwe\\WindowsPackageManagerServerDev.exe";
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

std::filesystem::path GetKnownFolderPath(const KNOWNFOLDERID& id)
{
    wil::unique_cotaskmem_string knownFolder = nullptr;
    THROW_IF_FAILED(SHGetKnownFolderPath(id, KF_FLAG_NO_ALIAS | KF_FLAG_DONT_VERIFY | KF_FLAG_NO_PACKAGE_REDIRECTION, NULL, &knownFolder));
    return knownFolder.get();
}

struct FreeWithRpcStringFree { void operator()(RPC_CSTR* in) { RpcStringFreeA(in); } };
using UniqueRpcString = std::unique_ptr<RPC_CSTR, FreeWithRpcStringFree>;

struct DeleteWithMidlFree { void operator()(void* m) { MIDL_user_free(m); } };
using UniqueMidl = std::unique_ptr<BYTE, DeleteWithMidlFree>;

void InitializeRpcBinding()
{
    std::string protocol = "ncacn_np";
    std::string endpoint = "\\pipe\\WinGetServerManualActivation_" + GetUserSID();

    unsigned char* binding = nullptr;
    UniqueRpcString bindingPtr;

    RPC_STATUS status = RpcStringBindingComposeA(nullptr, GetUCharString(protocol), nullptr, GetUCharString(endpoint), nullptr, &binding);
    THROW_HR_IF(HRESULT_FROM_WIN32(status), status != RPC_S_OK);
    bindingPtr.reset(&binding);

    status = RpcBindingFromStringBindingA(binding, &WinGetServerManualActivation_IfHandle);
    THROW_HR_IF(HRESULT_FROM_WIN32(status), status != RPC_S_OK);
}

HRESULT LaunchWinGetServerWithManualActivation()
{
    const std::filesystem::path& localAppDataPath = GetKnownFolderPath(FOLDERID_LocalAppData);
    const std::filesystem::path& serverExePath = localAppDataPath / s_ServerExePath;
    std::wstring commandLineInput = std::wstring{ serverExePath } + L" --manualActivation";

    STARTUPINFO info = { sizeof(info) };
    wil::unique_process_information process;

    RETURN_LAST_ERROR_IF(!CreateProcessW(NULL, &commandLineInput[0], NULL, NULL, FALSE, 0, NULL, NULL, &info, &process));

    // Wait for manual reset event from server before proceeding with COM activation.
    wil::unique_event manualResetEvent = CreateOrOpenServerStartEvent();
    manualResetEvent.wait(10000);

    return S_OK;
}

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

    static std::once_flag rpcBindingOnce;
    try
    {
        std::call_once(rpcBindingOnce, InitializeRpcBinding);
    }
    CATCH_RETURN();

    HRESULT result = CreateComInstance(rclsid, riid, flags, out);
    if (FAILED(result))
    {
        for (int i = 0; i < 3; i++)
        {
            result = LaunchWinGetServerWithManualActivation();
            if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
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
