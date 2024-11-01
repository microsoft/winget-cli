// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Factory.h"
#include <winrt/Microsoft.Management.Deployment.h>
#include <winget/Runtime.h>
#include <WinGetServerManualActivation_Client.h>

using namespace std::string_view_literals;

namespace Microsoft::Management::Deployment::OutOfProc
{
    namespace
    {
#if USE_PROD_CLSIDS
        constexpr CLSID CLSID_PackageManager = { 0xC53A4F16, 0x787E, 0x42A4, { 0xB3, 0x04, 0x29, 0xEF, 0xFB, 0x4B, 0xF5, 0x97 } }; //C53A4F16-787E-42A4-B304-29EFFB4BF597
        constexpr CLSID CLSID_InstallOptions = { 0x1095f097, 0xEB96, 0x453B, { 0xB4, 0xE6, 0x16, 0x13, 0x63, 0x7F, 0x3B, 0x14 } }; //1095F097-EB96-453B-B4E6-1613637F3B14
        constexpr CLSID CLSID_UninstallOptions = { 0xE1D9A11E, 0x9F85, 0x4D87, { 0x9C, 0x17, 0x2B, 0x93, 0x14, 0x3A, 0xDB, 0x8D } }; //E1D9A11E-9F85-4D87-9C17-2B93143ADB8D
        constexpr CLSID CLSID_FindPackagesOptions = { 0x572DED96, 0x9C60, 0x4526, { 0x8F, 0x92, 0xEE, 0x7D, 0x91, 0xD3, 0x8C, 0x1A } }; //572DED96-9C60-4526-8F92-EE7D91D38C1A
        constexpr CLSID CLSID_PackageMatchFilter = { 0xD02C9DAF, 0x99DC, 0x429C, { 0xB5, 0x03, 0x4E, 0x50, 0x4E, 0x4A, 0xB0, 0x00 } }; //D02C9DAF-99DC-429C-B503-4E504E4AB000
        constexpr CLSID CLSID_CreateCompositePackageCatalogOptions = { 0x526534B8, 0x7E46, 0x47C8, { 0x84, 0x16, 0xB1, 0x68, 0x5C, 0x32, 0x7D, 0x37 } }; //526534B8-7E46-47C8-8416-B1685C327D37
        constexpr CLSID CLSID_DownloadOptions = { 0x4CBABE76, 0x7322, 0x4BE4, { 0x9C, 0xEA, 0x25, 0x89, 0xA8, 0x06, 0x82, 0xDC } }; //4CBABE76-7322-4BE4-9CEA-2589A80682DC
        constexpr CLSID CLSID_AuthenticationArguments = { 0xBA580786, 0xBDE3, 0x4F6C, { 0xB8, 0xF3, 0x44, 0x69, 0x8A, 0xC8, 0x71, 0x1A } }; //BA580786-BDE3-4F6C-B8F3-44698AC8711A
        constexpr CLSID CLSID_RepairOptions = { 0x0498F441, 0x3097, 0x455F, { 0x9C, 0xAF, 0x14, 0x8F, 0x28, 0x29, 0x38, 0x65 } }; //0498F441-3097-455F-9CAF-148F28293865
        constexpr CLSID CLSID_AddPackageCatalogOptions = { 0xDB9D012D, 0x00D7, 0x47EE, { 0x8F, 0xB1, 0x60, 0x6E, 0x10, 0xAC, 0x4F, 0x51 } }; //DB9D012D-00D7-47EE-8FB1-606E10AC4F51
        constexpr CLSID CLSID_RemovePackageCatalogOptions = { 0x032B1C58, 0xB975, 0x469B, { 0xA0, 0x13, 0xE6, 0x32, 0xB6, 0xEC, 0xE8, 0xD8 } }; //032B1C58-B975-469B-A013-E632B6ECE8D8
#else
        constexpr CLSID CLSID_PackageManager = { 0x74CB3139, 0xB7C5, 0x4B9E, { 0x93, 0x88, 0xE6, 0x61, 0x6D, 0xEA, 0x28, 0x8C } }; //74CB3139-B7C5-4B9E-9388-E6616DEA288C
        constexpr CLSID CLSID_InstallOptions = { 0x44FE0580, 0x62F7, 0x44D4, { 0x9E, 0x91, 0xAA, 0x96, 0x14, 0xAB, 0x3E, 0x86 } }; //44FE0580-62F7-44D4-9E91-AA9614AB3E86
        constexpr CLSID CLSID_UninstallOptions = { 0xAA2A5C04, 0x1AD9, 0x46C4, { 0xB7, 0x4F, 0x6B, 0x33, 0x4A, 0xD7, 0xEB, 0x8C } }; //AA2A5C04-1AD9-46C4-B74F-6B334AD7EB8C
        constexpr CLSID CLSID_FindPackagesOptions = { 0x1BD8FF3A, 0xEC50, 0x4F69, { 0xAE, 0xEE, 0xDF, 0x4C, 0x9D, 0x3B, 0xAA, 0x96 } }; //1BD8FF3A-EC50-4F69-AEEE-DF4C9D3BAA96
        constexpr CLSID CLSID_PackageMatchFilter = { 0x3F85B9F4, 0x487A, 0x4C48, { 0x90, 0x35, 0x29, 0x03, 0xF8, 0xA6, 0xD9, 0xE8 } }; //3F85B9F4-487A-4C48-9035-2903F8A6D9E8
        constexpr CLSID CLSID_CreateCompositePackageCatalogOptions = { 0xEE160901, 0xB317, 0x4EA7, { 0x9C, 0xC6, 0x53, 0x55, 0xC6, 0xD7, 0xD8, 0xA7 } }; //EE160901-B317-4EA7-9CC6-5355C6D7D8A7
        constexpr CLSID CLSID_DownloadOptions = { 0x8EF324ED, 0x367C, 0x4880, { 0x83, 0xE5, 0xBB, 0x2A, 0xBD, 0x0B, 0x72, 0xF6 } }; //8EF324ED-367C-4880-83E5-BB2ABD0B72F6
        constexpr CLSID CLSID_AuthenticationArguments = { 0x6484A61D, 0x50FA, 0x41F0, { 0xB7, 0x1E, 0xF4, 0x37, 0x0C, 0x6E, 0xB3, 0x7C } }; //6484A61D-50FA-41F0-B71E-F4370C6EB37C
        constexpr CLSID CLSID_RepairOptions = { 0xE62BB1E7, 0xC7B2, 0x4AEC, { 0x9E, 0x28, 0xFB, 0x64, 0x9B, 0x30, 0xFF, 0x03 } }; //E62BB1E7-C7B2-4AEC-9E28-FB649B30FF03
        constexpr CLSID CLSID_AddPackageCatalogOptions = { 0xD58C7E4C, 0x70E6, 0x476C, { 0xA5, 0xD4, 0x80, 0x34, 0x1E, 0xD8, 0x02, 0x52 } }; //D58C7E4C-70E6-476C-A5D4-80341ED80252
        constexpr CLSID CLSID_RemovePackageCatalogOptions = { 0x87A96609, 0x1A39, 0x4955, { 0xBE, 0x72, 0x71, 0x74, 0xE1, 0x47, 0xB7, 0xDC } }; //87A96609-1A39-4955-BE72-7174E147B7DC

#endif

        struct NameCLSIDPair
        {
            std::wstring_view Name;
            GUID CLSID;
        };

        constexpr std::array<NameCLSIDPair, 11> s_nameCLSIDPairs
        {
            NameCLSIDPair{ L"Microsoft.Management.Deployment.PackageManager"sv, CLSID_PackageManager },
            NameCLSIDPair{ L"Microsoft.Management.Deployment.InstallOptions"sv, CLSID_InstallOptions },
            NameCLSIDPair{ L"Microsoft.Management.Deployment.UninstallOptions"sv, CLSID_UninstallOptions },
            NameCLSIDPair{ L"Microsoft.Management.Deployment.FindPackagesOptions"sv, CLSID_FindPackagesOptions },
            NameCLSIDPair{ L"Microsoft.Management.Deployment.PackageMatchFilter"sv, CLSID_PackageMatchFilter },
            NameCLSIDPair{ L"Microsoft.Management.Deployment.CreateCompositePackageCatalogOptions"sv, CLSID_CreateCompositePackageCatalogOptions },
            NameCLSIDPair{ L"Microsoft.Management.Deployment.DownloadOptions"sv, CLSID_DownloadOptions },
            NameCLSIDPair{ L"Microsoft.Management.Deployment.AuthenticationArguments"sv, CLSID_AuthenticationArguments },
            NameCLSIDPair{ L"Microsoft.Management.Deployment.RepairOptions"sv, CLSID_RepairOptions },
            NameCLSIDPair{ L"Microsoft.Management.Deployment.AddPackageCatalogOptions"sv, CLSID_AddPackageCatalogOptions },
            NameCLSIDPair{ L"Microsoft.Management.Deployment.RemovePackageCatalogOptions"sv, CLSID_RemovePackageCatalogOptions },
        };

        bool IsCLSIDPresent(const GUID& clsid)
        {
            for (const auto& pair : s_nameCLSIDPairs)
            {
                if (pair.CLSID == clsid)
                {
                    return true;
                }
            }

            return false;
        }

        const GUID* GetCLSIDFor(HSTRING clsid)
        {
            UINT32 length = 0;
            PCWSTR buffer = WindowsGetStringRawBuffer(clsid, &length);
            std::wstring_view clsidView{ buffer, length };

            for (const auto& pair : s_nameCLSIDPairs)
            {
                if (pair.Name == clsidView)
                {
                    return &pair.CLSID;
                }
            }

            return nullptr;
        }

        winrt::Windows::Foundation::IInspectable CreateOOPObject(const GUID& clsid)
        {
            bool isAdmin = AppInstaller::Runtime::IsRunningAsAdmin();

            try
            {
                return winrt::create_instance<winrt::Windows::Foundation::IInspectable>(clsid, CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD);
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
            THROW_IF_FAILED(WinGetServerManualActivation_CreateInstance(clsid, winrt::guid_of<winrt::Windows::Foundation::IInspectable>(), 0, result.put_void()));
            return result.as<winrt::Windows::Foundation::IInspectable>();
        }
    }

    Factory::Factory(const GUID& clsid) : m_clsid(clsid)
    {
        IncrementRefCount();
    }

    Factory::Factory(HSTRING clsid) : m_clsid(*GetCLSIDFor(clsid))
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
        return IsCLSIDPresent(clsid);
    }

    bool Factory::IsCLSID(HSTRING clsid)
    {
        return GetCLSIDFor(clsid) != nullptr;
    }

    winrt::Windows::Foundation::IInspectable Factory::ActivateInstance()
    {
        return CreateOOPObject(m_clsid);
    }

    HRESULT STDMETHODCALLTYPE Factory::CreateInstance(::IUnknown* pUnkOuter, REFIID riid, void** ppvObject) try
    {
        RETURN_HR_IF(E_POINTER, !ppvObject);
        *ppvObject = nullptr;
        RETURN_HR_IF(CLASS_E_NOAGGREGATION, pUnkOuter != nullptr);

        return CreateOOPObject(m_clsid).as(riid, ppvObject);
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
