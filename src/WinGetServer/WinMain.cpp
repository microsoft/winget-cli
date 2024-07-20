// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#define NOMINMAX
#pragma warning( push )
#pragma warning ( disable : 6001 6388 6553)
#include <wil/resource.h>
#include <wil/com.h>
#pragma warning( pop )
#include <objidl.h>
#include <shellapi.h>
#include <sddl.h>
#include <WindowsPackageManager.h>
#include "WinGetServer.h"
#include "Utils.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

// Holds the wwinmain open until COM tells us there are no more server connections
wil::unique_event _comServerExitEvent;

// Routine Description:
// - Called back when COM says there is nothing left for our server to do and we can tear down.
static void _releaseNotifier() noexcept
{
    _comServerExitEvent.SetEvent();
}

HRESULT WindowsPackageManagerServerInitializeRPCServer()
{
    std::string userSID = GetUserSID();
    std::string endpoint = "\\pipe\\WinGetServerManualActivation_" + userSID;
    RPC_STATUS status = RpcServerUseProtseqEpA(GetUCharString("ncacn_np"), RPC_C_PROTSEQ_MAX_REQS_DEFAULT, GetUCharString(endpoint), nullptr);
    RETURN_HR_IF(HRESULT_FROM_WIN32(status), status != RPC_S_OK);

    // The goal of this security descriptor is to restrict RPC server access only to the user in admin mode. 
    // (ML;;NW;;;HI) specifies a high mandatory integrity level (requires admin).
    // (A;;GA;;;UserSID) specifies access only for the user with the user SID (i.e. self).
    wil::unique_hlocal_security_descriptor securityDescriptor;
    std::string securityDescriptorString = "S:(ML;;NW;;;HI)D:(A;;GA;;;" + userSID + ")";
    RETURN_LAST_ERROR_IF(!ConvertStringSecurityDescriptorToSecurityDescriptorA(securityDescriptorString.c_str(), SDDL_REVISION_1, &securityDescriptor, nullptr));

    status = RpcServerRegisterIf3(WinGetServerManualActivation_v1_0_s_ifspec, nullptr, nullptr, RPC_IF_ALLOW_LOCAL_ONLY | RPC_IF_AUTOLISTEN, RPC_C_LISTEN_MAX_CALLS_DEFAULT, 0, nullptr, securityDescriptor.get());
    RETURN_HR_IF(HRESULT_FROM_WIN32(status), status != RPC_S_OK);

    return S_OK;
}

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

extern "C" HRESULT CreateInstance(
    /* [in] */ GUID clsid,
    /* [in] */ GUID iid,
    /* [in] */ UINT32,
    /* [ref][out] */ UINT32 * pcbBuffer,
    /* [size_is][size_is][ref][out] */ BYTE * *ppBuffer)
{
    RETURN_HR_IF_NULL(E_POINTER, pcbBuffer);
    RETURN_HR_IF_NULL(E_POINTER, ppBuffer);

    wil::com_ptr<IStream> stream;
    RETURN_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream));

    wil::com_ptr<IUnknown> instance;
    RETURN_IF_FAILED(WindowsPackageManagerServerCreateInstance(clsid, iid, reinterpret_cast<void**>(&instance)));

    RETURN_IF_FAILED(CoMarshalInterface(stream.get(), iid, instance.get(), MSHCTX_LOCAL, nullptr, MSHLFLAGS_NORMAL));

    ULARGE_INTEGER streamSize{};
    RETURN_IF_FAILED(stream->Seek({}, STREAM_SEEK_CUR, &streamSize));
    RETURN_HR_IF(E_NOT_SUFFICIENT_BUFFER, streamSize.QuadPart > std::numeric_limits<UINT32>::max());

    UINT32 bufferSize = static_cast<UINT32>(streamSize.QuadPart);

    struct DeleteWithMidlFree { void operator()(void* m) { MIDL_user_free(m); } };
    std::unique_ptr<BYTE, DeleteWithMidlFree> buffer{ reinterpret_cast<BYTE*>(MIDL_user_allocate(bufferSize)) };

    RETURN_IF_FAILED(stream->Seek({}, STREAM_SEEK_SET, nullptr));
    ULONG bytesRead = 0;
    RETURN_IF_FAILED(stream->Read(buffer.get(), bufferSize, &bytesRead));
    RETURN_HR_IF(E_UNEXPECTED, bytesRead != bufferSize);

    *pcbBuffer = bufferSize;
    *ppBuffer = buffer.release();

    return S_OK;
}

HRESULT InitializeComSecurity()
{
    wil::unique_hlocal_security_descriptor securityDescriptor;
    // Allow Self, System, Built-in Admin and App Container access. 3 is COM_RIGHTS_EXECUTE | COM_RIGHTS_EXECUTE_LOCAL
    std::string securityDescriptorString = "O:SYG:SYD:(A;;3;;;PS)(A;;3;;;SY)(A;;3;;;BA)(A;;3;;;AC)";
    RETURN_LAST_ERROR_IF(!ConvertStringSecurityDescriptorToSecurityDescriptorA(securityDescriptorString.c_str(), SDDL_REVISION_1, &securityDescriptor, nullptr));

    // Make absolute security descriptor as CoInitializeSecurity required
    SECURITY_DESCRIPTOR absoluteSecurityDescriptor;
    DWORD securityDescriptorSize = sizeof(SECURITY_DESCRIPTOR);

    DWORD daclSize = 0;
    DWORD saclSize = 0;
    DWORD ownerSize = 0;
    DWORD groupSize = 0;

    // Get required size
    BOOL result = MakeAbsoluteSD(securityDescriptor.get(), &absoluteSecurityDescriptor, &securityDescriptorSize, nullptr, &daclSize, nullptr, &saclSize, nullptr, &ownerSize, nullptr, &groupSize);
    RETURN_HR_IF_MSG(E_FAIL, result || GetLastError() != ERROR_INSUFFICIENT_BUFFER, "MakeAbsoluteSD failed to return buffer sizes");

    std::vector<BYTE> dacl(daclSize);
    std::vector<BYTE> sacl(saclSize);
    std::vector<BYTE> owner(ownerSize);
    std::vector<BYTE> group(groupSize);

    RETURN_LAST_ERROR_IF(!MakeAbsoluteSD(securityDescriptor.get(), &absoluteSecurityDescriptor, &securityDescriptorSize, (PACL)dacl.data(), &daclSize, (PACL)sacl.data(), &saclSize, (PACL)owner.data(), &ownerSize, (PACL)group.data(), &groupSize));

    // Initialize com security
    RETURN_IF_FAILED(CoInitializeSecurity(
        &absoluteSecurityDescriptor, // Security descriptor
        -1, // Authentication services count. -1 is let com choose.
        nullptr, // Authentication services array
        nullptr, // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT, // Authentication level.
        RPC_C_IMP_LEVEL_IDENTIFY, // Impersonation level. Identify client.
        nullptr, // Authentication list
        EOAC_NONE, // Additional capabilities
        nullptr // Reserved
    ));

    return S_OK;
}

int __stdcall wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR cmdLine, _In_ int)
{
    wil::SetResultLoggingCallback(&WindowsPackageManagerServerWilResultLoggingCallback);

    RETURN_IF_FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

    // Enable fast rundown of objects so that the server exits faster when clients go away.
    {
        wil::com_ptr<IGlobalOptions> globalOptions;
        RETURN_IF_FAILED(CoCreateInstance(CLSID_GlobalOptions, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&globalOptions)));
        RETURN_IF_FAILED(globalOptions->Set(COMGLB_RO_SETTINGS, COMGLB_FAST_RUNDOWN));
        RETURN_IF_FAILED(globalOptions->Set(COMGLB_UNMARSHALING_POLICY, COMGLB_UNMARSHALING_POLICY_STRONG));
        RETURN_IF_FAILED(globalOptions->Set(COMGLB_EXCEPTION_HANDLING, COMGLB_EXCEPTION_DONOT_HANDLE_ANY));
    }

    // Command line parsing
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(cmdLine, &argc);
    RETURN_LAST_ERROR_IF(!argv);

    bool manualActivation = false;

    // If command line gets more complicated, consider more complex parsing
    if (argc == 1 && std::wstring_view{ L"--manualActivation" } == argv[0])
    {
        manualActivation = true;
    }

    // For packaged com activation, initialize com security.
    // For manual activation, leave as default. We'll not register objects for manual activation.
    if (!manualActivation)
    {
        // This must be called after IGlobalOptions (fast rundown setting cannot be changed after CoInitializeSecurity)
        // This must be called before WindowsPackageManagerServerInitialize (when setting the logs
        // to Windows.Storage folders, automatic CoInitializeSecurity is triggered)
        RETURN_IF_FAILED(InitializeComSecurity());
    }

    RETURN_IF_FAILED(WindowsPackageManagerServerInitialize());

    _comServerExitEvent.create();
    RETURN_IF_FAILED(WindowsPackageManagerServerModuleCreate(&_releaseNotifier));
    try
    {
        // Manual reset event to notify the client that the server is available.
        wil::unique_event manualResetEvent;

        if (manualActivation)
        {
            // For manual activation, do not register com objects
            // so that only RPC channel can be used.
            HANDLE hMutex = NULL;
            hMutex = CreateMutex(NULL, FALSE, TEXT("WinGetServerMutex"));
            RETURN_LAST_ERROR_IF_NULL(hMutex);

            DWORD waitResult = WaitForSingleObject(hMutex, 0);
            if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_ABANDONED)
            {
                return HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING);
            }

            RETURN_IF_FAILED(WindowsPackageManagerServerInitializeRPCServer());

            manualResetEvent = CreateOrOpenServerStartEvent();
            manualResetEvent.SetEvent();
        }
        else
        {
            // Register all the CoCreatableClassWrlCreatorMapInclude classes
            RETURN_IF_FAILED(WindowsPackageManagerServerModuleRegister());
        }

        _comServerExitEvent.wait();

        if (manualResetEvent)
        {
            manualResetEvent.reset();
        }

        if (!manualActivation)
        {
            RETURN_IF_FAILED(WindowsPackageManagerServerModuleUnregister());
        }
    }
    CATCH_RETURN()

    return 0;
}
