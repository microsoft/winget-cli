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
    std::string endpoint = GetServerEndpointName();
    RPC_STATUS status = RpcServerUseProtseqEpA(GetUCharString("ncalrpc"), RPC_C_PROTSEQ_MAX_REQS_DEFAULT, GetUCharString(endpoint), nullptr);
    RETURN_HR_IF(HRESULT_FROM_WIN32(status), status != RPC_S_OK);

    // The goal of this security descriptor is to restrict RPC server access only to the user in admin mode.
    // It is enforced by RPC, which impersonates the caller
    // and runs AccessCheck with MAXIMUM_ALLOWED, denying the call only when the granted access is 0.
    // (A;;GA;;;UserSID) specifies access only for the user with the user SID (i.e. self).
    // (ML;;NRNWNX;;;HI) requires the caller to be at high integrity. All three of no-read-up,
    // no-write-up and no-execute-up are required: the access check uses a generic mapping of
    // STANDARD_RIGHTS_READ/WRITE/EXECUTE, so a no-write-up policy alone would still leave a
    // medium integrity caller with the read and execute rights granted by GENERIC_ALL above,
    // producing a non-zero granted access and allowing the call.
    wil::unique_hlocal_security_descriptor securityDescriptor;
    std::string securityDescriptorString = "D:(A;;GA;;;" + userSID + ")S:(ML;;NRNWNX;;;HI)";

#ifndef AICLI_DISABLE_TEST_HOOKS
    // When running at non-admin integrity (e.g. a medium-integrity server process spawned by
    // the security E2E tests to validate client-side rejection), omit the mandatory label SACL.
    // A medium-integrity process cannot set a high integrity label on the interface. When
    // running elevated, use the full production SD so the elevated-client positive test also
    // exercises the real security configuration.
    {
        BOOL isAdmin = FALSE;
        {
            PSID adminGroup = nullptr;
            SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
            if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup))
            {
                CheckTokenMembership(nullptr, adminGroup, &isAdmin);
                FreeSid(adminGroup);
            }
        }

        if (!isAdmin)
        {
            securityDescriptorString = "D:(A;;GA;;;" + userSID + ")";
        }
    }
#endif

    RETURN_LAST_ERROR_IF(!ConvertStringSecurityDescriptorToSecurityDescriptorA(securityDescriptorString.c_str(), SDDL_REVISION_1, &securityDescriptor, nullptr));

    // ncalrpc only supports RPC_C_AUTHN_WINNT; RPC_C_AUTHN_GSS_NEGOTIATE is rejected by the
    // LRPC binding handle with RPC_S_UNKNOWN_AUTHN_SERVICE.
    status = RpcServerRegisterAuthInfoA(nullptr, RPC_C_AUTHN_WINNT, nullptr, nullptr);
    RETURN_HR_IF(HRESULT_FROM_WIN32(status), status != RPC_S_OK);

    // Note: RPC_IF_ALLOW_SECURE_ONLY and MinAuthLevel have no effect on ncalrpc. They are retained as
    // defense in depth should the transport ever change.
    // Caller enforcement on this transport comes from the interface security descriptor above,
    // checked against the token the kernel attaches to the ALPC message, which a caller cannot forge.
    status = RpcServerRegisterIf3(WinGetServerManualActivation_v1_0_s_ifspec, nullptr, nullptr, RPC_IF_ALLOW_LOCAL_ONLY | RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_SECURE_ONLY,
        RPC_C_LISTEN_MAX_CALLS_DEFAULT, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, nullptr, securityDescriptor.get());
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
        WindowsPackageManagerServerLog("Server shutting down after exit event signaled.");

        if (manualResetEvent)
        {
            manualResetEvent.reset();
        }

        if (!manualActivation)
        {
            RETURN_IF_FAILED(WindowsPackageManagerServerModuleUnregister());
        }
    }
    catch (...)
    {
        LOG_CAUGHT_EXCEPTION();
        RETURN_CAUGHT_EXCEPTION();
    }

    return 0;
}
