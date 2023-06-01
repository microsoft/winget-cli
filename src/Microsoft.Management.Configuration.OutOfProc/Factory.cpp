// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Factory.h"

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
        // TODO: Implement anything needed here
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

    HRESULT STDMETHODCALLTYPE Factory::ActivateInstance(::IInspectable** instance);

    HRESULT STDMETHODCALLTYPE Factory::CreateInstance(::IUnknown* pUnkOuter, REFIID riid, void** ppvObject);

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
