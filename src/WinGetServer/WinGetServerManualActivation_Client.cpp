﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "WinGetServer.h"

#include <wil/com.h>
#include <wil/safecast.h>

#include <memory>
#include <mutex>
#include <string>

#if PROD
const std::wstring& s_ServerExePath = L"\\Microsoft\\WindowsApps\\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\\WindowsPackageManagerServer.exe";
#else
const std::wstring& s_ServerExePath = L"\\Microsoft\\WindowsApps\\WinGetDevCLI_8wekyb3d8bbwe\\WindowsPackageManagerServerDev.exe";
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

unsigned char* GetUCharString(const std::string& str)
{
    return reinterpret_cast<unsigned char*>(const_cast<char*>(str.c_str()));
}

std::wstring ExpandEnvironmentVariables(const std::wstring& input)
{
    if (input.empty())
    {
        return {};
    }

    DWORD charCount = ExpandEnvironmentStringsW(input.c_str(), nullptr, 0);
    THROW_LAST_ERROR_IF(charCount == 0);

    std::wstring result(wil::safe_cast<size_t>(charCount), L'\0');

    DWORD charCountWritten = ExpandEnvironmentStringsW(input.c_str(), &result[0], charCount);
    THROW_HR_IF(E_UNEXPECTED, charCount != charCountWritten);

    if (result.back() == L'\0')
    {
        result.resize(result.size() - 1);
    }

    return result;
}

struct FreeWithRpcStringFree { void operator()(RPC_CSTR* in) { RpcStringFreeA(in); } };
using UniqueRpcString = std::unique_ptr<RPC_CSTR, FreeWithRpcStringFree>;

struct DeleteWithMidlFree { void operator()(void* m) { MIDL_user_free(m); } };
using UniqueMidl = std::unique_ptr<BYTE, DeleteWithMidlFree>;

void InitializeRpcBinding()
{
    std::string protocol = "ncacn_np";
    std::string endpoint = "\\pipe\\WinGetServerManualActivation_SID";

    unsigned char* binding = nullptr;
    UniqueRpcString bindingPtr;

    RPC_STATUS status = RpcStringBindingComposeA(nullptr, GetUCharString(protocol), nullptr, GetUCharString(endpoint), nullptr, &binding);
    THROW_HR_IF(HRESULT_FROM_WIN32(status), status != RPC_S_OK);
    bindingPtr.reset(&binding);

    status = RpcBindingFromStringBindingA(binding, &WinGetServerManualActivation_IfHandle);
    THROW_HR_IF(HRESULT_FROM_WIN32(status), status != RPC_S_OK);
}

void LaunchWinGetServerWithManualActivation()
{
    const std::wstring& localAppDataPath = ExpandEnvironmentVariables(L"%LOCALAPPDATA%");
    std::wstring serverExePath = localAppDataPath + s_ServerExePath + L" --manualActivation";

    STARTUPINFO info = { sizeof(info) };
    PROCESS_INFORMATION processInfo;
    CreateProcessW(NULL, &serverExePath[0], NULL, NULL, FALSE, 0, NULL, NULL, &info, &processInfo);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    Sleep(1000);
}

HRESULT CallCreateInstance(const CLSID* clsid, const IID* iid, UINT32 flags, UINT32* bufferByteCount, BYTE** buffer)
{
    RpcTryExcept
    {
        RETURN_IF_FAILED(CreateInstance(*clsid, *iid, flags, bufferByteCount, buffer));
    }
        RpcExcept(1)
    {
        return HRESULT_FROM_WIN32(RpcExceptionCode());
    }
    RpcEndExcept;

    return S_OK;
}

extern "C" HRESULT WinGetServerManualActivation_CreateInstance(const CLSID* clsid, const IID* iid, UINT32 flags, void** out)
{
    RETURN_HR_IF_NULL(E_POINTER, clsid);
    RETURN_HR_IF_NULL(E_POINTER, iid);
    RETURN_HR_IF_NULL(E_POINTER, out);

    static std::once_flag rpcBindingOnce;
    try
    {
        std::call_once(rpcBindingOnce, InitializeRpcBinding);
    }
    CATCH_RETURN();

    UINT32 bufferByteCount = 0;
    BYTE* buffer = nullptr;
    UniqueMidl bufferPtr;

    if (FAILED(CallCreateInstance(clsid, iid, flags, &bufferByteCount, &buffer)))
    {
        LaunchWinGetServerWithManualActivation();
        RETURN_IF_FAILED(CallCreateInstance(clsid, iid, flags, &bufferByteCount, &buffer));
    }

    bufferPtr.reset(buffer);

    wil::com_ptr<IStream> stream;
    RETURN_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream));
    RETURN_IF_FAILED(stream->Write(buffer, bufferByteCount, nullptr));
    RETURN_IF_FAILED(stream->Seek({}, STREAM_SEEK_SET, nullptr));

    wil::com_ptr<IUnknown> output;
    RETURN_IF_FAILED(CoUnmarshalInterface(stream.get(), *iid, reinterpret_cast<void**>(&output)));

    *out = output.detach();
    return S_OK;
}