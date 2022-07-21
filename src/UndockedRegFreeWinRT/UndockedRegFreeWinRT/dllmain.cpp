#include <Windows.h>
#include <synchapi.h>
#include <roapi.h>
#include <windows.foundation.h>
#include <activationregistration.h>
#include <combaseapi.h>
#include <wrl.h>
#include <ctxtcall.h>
#include <Processthreadsapi.h>
#include <activation.h>
#include <hstring.h>
#include <VersionHelpers.h>
#include <memory>
#include "../detours/detours.h"
#include "catalog.h"
#include "extwinrt.h"
#include "TypeResolution.h"

#define WIN1019H1_BLDNUM 18362

// Ensure that metadata resolution functions are imported so they can be detoured
extern "C"
{
    __declspec(dllimport) HRESULT WINAPI RoGetMetaDataFile(
        const HSTRING name,
        IMetaDataDispenserEx* metaDataDispenser,
        HSTRING* metaDataFilePath,
        IMetaDataImport2** metaDataImport,
        mdTypeDef* typeDefToken);

    __declspec(dllimport) HRESULT WINAPI RoParseTypeName(
        HSTRING typeName,
        DWORD* partsCount,
        HSTRING** typeNameParts);

    __declspec(dllimport) HRESULT WINAPI RoResolveNamespace(
        const HSTRING name,
        const HSTRING windowsMetaDataDir,
        const DWORD packageGraphDirsCount,
        const HSTRING* packageGraphDirs,
        DWORD* metaDataFilePathsCount,
        HSTRING** metaDataFilePaths,
        DWORD* subNamespacesCount,
        HSTRING** subNamespaces);

    __declspec(dllimport) HRESULT WINAPI RoIsApiContractPresent(
        PCWSTR name,
        UINT16 majorVersion,
        UINT16 minorVersion,
        BOOL* present);

    __declspec(dllimport) HRESULT WINAPI RoIsApiContractMajorVersionPresent(
        PCWSTR name,
        UINT16 majorVersion,
        BOOL* present);
}

static decltype(RoActivateInstance)* TrueRoActivateInstance = RoActivateInstance;
static decltype(RoGetActivationFactory)* TrueRoGetActivationFactory = RoGetActivationFactory;
static decltype(RoGetMetaDataFile)* TrueRoGetMetaDataFile = RoGetMetaDataFile;
static decltype(RoResolveNamespace)* TrueRoResolveNamespace = RoResolveNamespace;

enum class ActivationLocation
{
    CurrentApartment,
    CrossApartmentMTA
};

VERSIONHELPERAPI IsWindowsVersionOrGreaterEx(WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor, WORD wBuildNumber)
{
    OSVERSIONINFOEXW osvi = { sizeof(osvi) };
    DWORDLONG const dwlConditionMask =
        VerSetConditionMask(
            VerSetConditionMask(
                VerSetConditionMask(
                    VerSetConditionMask(
                        0, VER_MAJORVERSION, VER_GREATER_EQUAL),
                    VER_MINORVERSION, VER_GREATER_EQUAL),
                VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL),
            VER_BUILDNUMBER, VER_GREATER_EQUAL);

    osvi.dwMajorVersion = wMajorVersion;
    osvi.dwMinorVersion = wMinorVersion;
    osvi.wServicePackMajor = wServicePackMajor;
    osvi.dwBuildNumber = wBuildNumber;

    return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_BUILDNUMBER, dwlConditionMask) != FALSE;
}

VERSIONHELPERAPI IsWindows1019H1OrGreater()
{
    return IsWindowsVersionOrGreaterEx(HIBYTE(_WIN32_WINNT_WIN10), LOBYTE(_WIN32_WINNT_WIN10), 0, WIN1019H1_BLDNUM);
}

VOID CALLBACK EnsureMTAInitializedCallBack
(
    PTP_CALLBACK_INSTANCE instance,
    PVOID                 parameter,
    PTP_WORK              work
)
{
    Microsoft::WRL::ComPtr<IComThreadingInfo> spThreadingInfo;
    CoGetObjectContext(IID_PPV_ARGS(&spThreadingInfo));
}

/* 
In the context callback call to the MTA apartment, there is a bug that prevents COM 
from automatically initializing MTA remoting. It only allows NTA to be intialized 
outside of the NTA and blocks all others. The workaround for this is to spin up another 
thread that is not been CoInitialize. COM treats this thread as a implicit MTA and 
when we call CoGetObjectContext on it we implicitily initialized the MTA. 
*/
HRESULT EnsureMTAInitialized()
{
    TP_CALLBACK_ENVIRON callBackEnviron;
    InitializeThreadpoolEnvironment(&callBackEnviron);
    PTP_POOL pool = CreateThreadpool(nullptr);
    if (pool == nullptr)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    SetThreadpoolThreadMaximum(pool, 1);
    if (!SetThreadpoolThreadMinimum(pool, 1)) 
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    PTP_CLEANUP_GROUP cleanupgroup = CreateThreadpoolCleanupGroup();
    if (cleanupgroup == nullptr)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    SetThreadpoolCallbackPool(&callBackEnviron, pool);
    SetThreadpoolCallbackCleanupGroup(&callBackEnviron,
        cleanupgroup,
        nullptr);
    PTP_WORK ensureMTAInitializedWork = CreateThreadpoolWork(
        &EnsureMTAInitializedCallBack,
        nullptr,
        &callBackEnviron);
    if (ensureMTAInitializedWork == nullptr)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    SubmitThreadpoolWork(ensureMTAInitializedWork);
    CloseThreadpoolCleanupGroupMembers(cleanupgroup,
        false,
        nullptr);
    return S_OK;
}

HRESULT GetActivationLocation(HSTRING activatableClassId, ActivationLocation &activationLocation)
{
    APTTYPE aptType;
    APTTYPEQUALIFIER aptQualifier;
    RETURN_IF_FAILED(CoGetApartmentType(&aptType, &aptQualifier));

    ABI::Windows::Foundation::ThreadingType threading_model;
    RETURN_IF_FAILED(WinRTGetThreadingModel(activatableClassId, &threading_model)); //REGDB_E_CLASSNOTREG
    switch (threading_model)
    {
    case ABI::Windows::Foundation::ThreadingType_BOTH:
        activationLocation = ActivationLocation::CurrentApartment;
        break;
    case ABI::Windows::Foundation::ThreadingType_STA:
        if (aptType == APTTYPE_MTA)
        {
            return RO_E_UNSUPPORTED_FROM_MTA;
        }
        else
        {
            activationLocation = ActivationLocation::CurrentApartment;
        }
        break;
    case ABI::Windows::Foundation::ThreadingType_MTA:
        if (aptType == APTTYPE_MTA)
        {
            activationLocation = ActivationLocation::CurrentApartment;
        }
        else
        {
            activationLocation = ActivationLocation::CrossApartmentMTA;
        }
        break;
    }
    return S_OK;
}


HRESULT WINAPI RoActivateInstanceDetour(HSTRING activatableClassId, IInspectable** instance)
{
    ActivationLocation location;
    HRESULT hr = GetActivationLocation(activatableClassId, location);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        return TrueRoActivateInstance(activatableClassId, instance);
    }
    RETURN_IF_FAILED(hr);

    // Activate in current apartment
    if (location == ActivationLocation::CurrentApartment)
    {
        Microsoft::WRL::ComPtr<IActivationFactory> pFactory;
        RETURN_IF_FAILED(WinRTGetActivationFactory(activatableClassId, __uuidof(IActivationFactory), (void**)&pFactory));
        return pFactory->ActivateInstance(instance);
    }

    // Cross apartment MTA activation
    struct CrossApartmentMTAActData {
        HSTRING activatableClassId;
        IStream *stream;
    };

    CrossApartmentMTAActData cbdata{ activatableClassId };
    CO_MTA_USAGE_COOKIE mtaUsageCookie;
    RETURN_IF_FAILED(CoIncrementMTAUsage(&mtaUsageCookie));
    RETURN_IF_FAILED(EnsureMTAInitialized());
    Microsoft::WRL::ComPtr<IContextCallback> defaultContext;
    ComCallData data;
    data.pUserDefined = &cbdata;
    RETURN_IF_FAILED(CoGetDefaultContext(APTTYPE_MTA, IID_PPV_ARGS(&defaultContext)));
    RETURN_IF_FAILED(defaultContext->ContextCallback(
        [](_In_ ComCallData* pComCallData) -> HRESULT
        {
            CrossApartmentMTAActData* data = reinterpret_cast<CrossApartmentMTAActData*>(pComCallData->pUserDefined);
            Microsoft::WRL::ComPtr<IInspectable> instance;
            Microsoft::WRL::ComPtr<IActivationFactory> pFactory;
            RETURN_IF_FAILED(WinRTGetActivationFactory(data->activatableClassId, __uuidof(IActivationFactory), (void**)&pFactory));
            RETURN_IF_FAILED(pFactory->ActivateInstance(&instance));
            RETURN_IF_FAILED(CoMarshalInterThreadInterfaceInStream(IID_IInspectable, instance.Get(), &data->stream));
            return S_OK;
        },
        &data, IID_ICallbackWithNoReentrancyToApplicationSTA, 5, nullptr)); // 5 is meaningless.
    RETURN_IF_FAILED(CoGetInterfaceAndReleaseStream(cbdata.stream, IID_IInspectable, (LPVOID*)instance));
    return S_OK;
}

HRESULT WINAPI RoGetActivationFactoryDetour(HSTRING activatableClassId, REFIID iid, void** factory)
{
    ActivationLocation location;
    HRESULT hr = GetActivationLocation(activatableClassId, location);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        return TrueRoGetActivationFactory(activatableClassId, iid, factory);
    }
    RETURN_IF_FAILED(hr);

    // Activate in current apartment
    if (location == ActivationLocation::CurrentApartment)
    {
        RETURN_IF_FAILED(WinRTGetActivationFactory(activatableClassId, iid, factory));
        return S_OK;
    }
    // Cross apartment MTA activation
    struct CrossApartmentMTAActData {
        HSTRING activatableClassId;
        IStream* stream;
    };
    CrossApartmentMTAActData cbdata{ activatableClassId };
    CO_MTA_USAGE_COOKIE mtaUsageCookie;
    RETURN_IF_FAILED(CoIncrementMTAUsage(&mtaUsageCookie));
    RETURN_IF_FAILED(EnsureMTAInitialized());
    Microsoft::WRL::ComPtr<IContextCallback> defaultContext;
    ComCallData data;
    data.pUserDefined = &cbdata;
    RETURN_IF_FAILED(CoGetDefaultContext(APTTYPE_MTA, IID_PPV_ARGS(&defaultContext)));
    defaultContext->ContextCallback(
        [](_In_ ComCallData* pComCallData) -> HRESULT
        {
            CrossApartmentMTAActData* data = reinterpret_cast<CrossApartmentMTAActData*>(pComCallData->pUserDefined);
            Microsoft::WRL::ComPtr<IActivationFactory> pFactory;
            RETURN_IF_FAILED(WinRTGetActivationFactory(data->activatableClassId, __uuidof(IActivationFactory), (void**)&pFactory));
            RETURN_IF_FAILED(CoMarshalInterThreadInterfaceInStream(IID_IActivationFactory, pFactory.Get(), &data->stream));
            return S_OK;
        },
        &data, IID_ICallbackWithNoReentrancyToApplicationSTA, 5, nullptr); // 5 is meaningless.
    RETURN_IF_FAILED(CoGetInterfaceAndReleaseStream(cbdata.stream, IID_IActivationFactory, factory));
    return S_OK;
}

HRESULT WINAPI RoGetMetaDataFileDetour(
    const HSTRING name,
    IMetaDataDispenserEx* metaDataDispenser,
    HSTRING* metaDataFilePath,
    IMetaDataImport2** metaDataImport,
    mdTypeDef* typeDefToken)
{
    HRESULT hr = WinRTGetMetadataFile(name, metaDataDispenser, metaDataFilePath, metaDataImport, typeDefToken);
    // Don't fallback on RO_E_METADATA_NAME_IS_NAMESPACE failure. This is the intended behavior for namespace names.
    if (FAILED(hr) && hr != RO_E_METADATA_NAME_IS_NAMESPACE)
    {
        hr = TrueRoGetMetaDataFile(name, metaDataDispenser, metaDataFilePath, metaDataImport, typeDefToken);
    }
    return hr;
}

HRESULT WINAPI RoResolveNamespaceDetour(
    const HSTRING name,
    const HSTRING windowsMetaDataDir,
    const DWORD packageGraphDirsCount,
    const HSTRING* packageGraphDirs,
    DWORD* metaDataFilePathsCount,
    HSTRING** metaDataFilePaths,
    DWORD* subNamespacesCount,
    HSTRING** subNamespaces)
{
    PCWSTR exeFilePath = nullptr;
    UndockedRegFreeWinRT::GetProcessExeDir(&exeFilePath);
    auto pathReference = Microsoft::WRL::Wrappers::HStringReference(exeFilePath);
    PCWSTR dllFilePath = nullptr;
    UndockedRegFreeWinRT::GetProcessDllDir(&dllFilePath);
    auto dllPathReference = Microsoft::WRL::Wrappers::HStringReference(dllFilePath);
    HSTRING packageGraphDirectories[] = { pathReference.Get(), dllPathReference.Get() };
    HRESULT hr = TrueRoResolveNamespace(name, pathReference.Get(),
        ARRAYSIZE(packageGraphDirectories), packageGraphDirectories,
        metaDataFilePathsCount, metaDataFilePaths,
        subNamespacesCount, subNamespaces);
    if (FAILED(hr))
    {
        hr = TrueRoResolveNamespace(name, windowsMetaDataDir,
            packageGraphDirsCount, packageGraphDirs,
            metaDataFilePathsCount, metaDataFilePaths,
            subNamespacesCount, subNamespaces);
    }
    return hr;
}

void InstallHooks()
{
    if (DetourIsHelperProcess()) {
        return;
    }

    DetourRestoreAfterWith();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)TrueRoActivateInstance, RoActivateInstanceDetour);
    DetourAttach(&(PVOID&)TrueRoGetActivationFactory, RoGetActivationFactoryDetour);
    DetourAttach(&(PVOID&)TrueRoGetMetaDataFile, RoGetMetaDataFileDetour);
    DetourAttach(&(PVOID&)TrueRoResolveNamespace, RoResolveNamespaceDetour);
    DetourTransactionCommit();
}

void RemoveHooks()
{

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)TrueRoActivateInstance, RoActivateInstanceDetour);
    DetourDetach(&(PVOID&)TrueRoGetActivationFactory, RoGetActivationFactoryDetour);
    DetourDetach(&(PVOID&)TrueRoGetMetaDataFile, RoGetMetaDataFileDetour);
    DetourDetach(&(PVOID&)TrueRoResolveNamespace, RoResolveNamespaceDetour);
    DetourTransactionCommit();
}

HRESULT ExtRoLoadCatalog()
{
    WCHAR filePath[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, filePath, _countof(filePath)))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    std::wstring manifestPath(filePath);
    HANDLE hActCtx = INVALID_HANDLE_VALUE;
    auto exit = wil::scope_exit([&]
    {
        if (hActCtx != INVALID_HANDLE_VALUE)
        {
            ReleaseActCtx(hActCtx);
        }
    });
    wil::unique_hmodule exeModule;
    RETURN_IF_WIN32_BOOL_FALSE(GetModuleHandleExW(0, nullptr, &exeModule));
    ACTCTXW acw = { sizeof(acw) };
    acw.lpSource = manifestPath.c_str();
    acw.hModule = exeModule.get();
    acw.lpResourceName = MAKEINTRESOURCEW(1);
    acw.dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID;

    hActCtx = CreateActCtxW(&acw);
    RETURN_LAST_ERROR_IF(!hActCtx);
    if (hActCtx == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    PACTIVATION_CONTEXT_DETAILED_INFORMATION actCtxInfo = nullptr;
    SIZE_T bufferSize = 0;
    (void)QueryActCtxW(0,
        hActCtx,
        nullptr,
        ActivationContextDetailedInformation,
        nullptr,
        0,
        &bufferSize);
    RETURN_HR_IF(HRESULT_FROM_WIN32(GetLastError()), bufferSize == 0);
    auto actCtxInfoBuffer = std::make_unique<BYTE[]>(bufferSize);
    actCtxInfo = reinterpret_cast<PACTIVATION_CONTEXT_DETAILED_INFORMATION>(actCtxInfoBuffer.get());
    if (!actCtxInfo)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    RETURN_IF_WIN32_BOOL_FALSE(QueryActCtxW(0,
        hActCtx,
        nullptr,
        ActivationContextDetailedInformation,
        actCtxInfo,
        bufferSize,
        nullptr));

    for (DWORD index = 1; index <= actCtxInfo->ulAssemblyCount; index++)
    {
        bufferSize = 0;
        PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION asmInfo = nullptr;
        (void)QueryActCtxW(0,
            hActCtx,
            &index,
            AssemblyDetailedInformationInActivationContext,
            nullptr,
            0,
            &bufferSize);
        RETURN_HR_IF(HRESULT_FROM_WIN32(GetLastError()), bufferSize == 0);
        auto asmInfobuffer = std::make_unique<BYTE[]>(bufferSize);
        asmInfo = reinterpret_cast<PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION>(asmInfobuffer.get());
        if (!asmInfo)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return HRESULT_FROM_WIN32(GetLastError());
        }

        RETURN_IF_WIN32_BOOL_FALSE(QueryActCtxW(0,
            hActCtx,
            &index,
            AssemblyDetailedInformationInActivationContext,
            asmInfo,
            bufferSize,
            nullptr));
        RETURN_IF_FAILED(LoadManifestFromPath(asmInfo->lpAssemblyManifestPath));
    }
    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE hmodule, DWORD reason, LPVOID /*lpvReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hmodule);
        InstallHooks();
        try
        {
            ExtRoLoadCatalog();
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            return false;
        }
    }
    if (reason == DLL_PROCESS_DETACH)
    {
        RemoveHooks();
    }
    return true;
}

extern "C" void WINAPI winrtact_Initialize()
{
    return;
}
