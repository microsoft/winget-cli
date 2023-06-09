// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Factory.h"
#include <winrt/Microsoft.Management.Configuration.h>
#include <winget/Runtime.h>
#include <WinGetServerManualActivation_Client.h>

namespace Microsoft::Management::Configuration::OutOfProc
{
    namespace
    {
        const CLSID& GetConfigurationStaticsCLSID()
        {
#if USE_PROD_CLSIDS
            static const CLSID CLSID_ConfigurationStatics = { 0x73d763b7,0x2937,0x432f,{0xa9,0x7a,0xd9,0x8a,0x4a,0x59,0x61,0x26} };  // 73D763B7-2937-432F-A97A-D98A4A596126
#else
            static const CLSID CLSID_ConfigurationStatics = { 0xc9ed7917,0x66ab,0x4e31,{0xa9,0x2a,0xf6,0x5f,0x18,0xef,0x79,0x33} };  // C9ED7917-66AB-4E31-A92A-F65F18EF7933
#endif

            return CLSID_ConfigurationStatics;
        }

        winrt::Microsoft::Management::Configuration::IConfigurationStatics CreateOOPStaticsObject()
        {
            bool isAdmin = AppInstaller::Runtime::IsRunningAsAdmin();

            try
            {
                return winrt::create_instance<winrt::Microsoft::Management::Configuration::IConfigurationStatics>(GetConfigurationStaticsCLSID(), CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD);
            }
            catch (const winrt::hresult_error& hre)
            {
                // We only want to fall through to trying the manual activation if we are running as admin and couldn't find the registration.
                if (!(isAdmin && hre.code() == REGDB_E_CLASSNOTREG))
                {
                    throw;
                }
            }

            winrt::com_ptr<::IUnknown> result;
            THROW_IF_FAILED(WinGetServerManualActivation_CreateInstance(GetConfigurationStaticsCLSID(), winrt::guid_of<winrt::Microsoft::Management::Configuration::IConfigurationStatics>(), 0, result.put_void()));
            return result.as<winrt::Microsoft::Management::Configuration::IConfigurationStatics>();
        }
    }

    Factory::Factory()
    {
        IncrementRefCount();
    }

    Factory::~Factory()
    {
        DecrementRefCount();
    }

    bool Factory::HasReferences()
    {
        return s_referenceCount.load() != 0;
    }

    void Factory::Terminate()
    {
        WinGetServerManualActivation_Terminate();
    }

    bool Factory::IsCLSID(const GUID& clsid)
    {
        if (clsid == GetConfigurationStaticsCLSID())
        {
            return true;
        }

        return false;
    }

    bool Factory::IsCLSID(HSTRING clsid)
    {
        constexpr std::wstring_view s_ClassName = L"Microsoft.Management.Configuration.ConfigurationStaticFunctions";

        UINT32 length = 0;
        PCWSTR buffer = WindowsGetStringRawBuffer(clsid, &length);

        if (std::wstring_view{ buffer, length } == s_ClassName)
        {
            return true;
        }

        return false;
    }

    winrt::Windows::Foundation::IInspectable Factory::ActivateInstance()
    {
        return CreateOOPStaticsObject().as<winrt::Windows::Foundation::IInspectable>();
    }

    HRESULT STDMETHODCALLTYPE Factory::CreateInstance(::IUnknown* pUnkOuter, REFIID riid, void** ppvObject) try
    {
        RETURN_HR_IF(E_POINTER, !ppvObject);
        *ppvObject = nullptr;
        RETURN_HR_IF(CLASS_E_NOAGGREGATION, pUnkOuter != nullptr);

        return CreateOOPStaticsObject().as(riid, ppvObject);
    }
    CATCH_RETURN();

    HRESULT STDMETHODCALLTYPE Factory::LockServer(BOOL fLock)
    {
        if (fLock)
        {
            IncrementRefCount();
        }
        else
        {
            DecrementRefCount();
        }

        return S_OK;
    }

    void Factory::IncrementRefCount()
    {
        ++s_referenceCount;
    }

    void Factory::DecrementRefCount()
    {
        --s_referenceCount;
    }

    std::atomic<int32_t> Factory::s_referenceCount = ATOMIC_VAR_INIT(0);
}
