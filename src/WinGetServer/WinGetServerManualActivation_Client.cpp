// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "WinGetServer.h"
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

struct ServerProcessLauncher
{
    ServerProcessLauncher()
    {
        try
        {
            m_serverExePath = GetPackageLocation(s_ServerPackageFamilyName, s_ServerFileName) / s_ServerFileName;

#ifndef USE_PROD_WINGET_SERVER
            // The feature that allows directly launching a packaged process as long as it has a matching alias
            // requires a failure to trigger, and the dev package is not ACL'd to force this to happen. Attempting
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

        std::wstring commandLineInput = std::wstring{ m_serverExePath } + L" --manualActivation";

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
