// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/ComClsids.h"
#pragma warning( push )
#pragma warning ( disable : 4467 )
// 4467 Allow use of uuid attribute for com object creation.
#include "PackageManager.h"
#include "FindPackagesOptions.h"
#include "CreateCompositePackageCatalogOptions.h"
#include "InstallOptions.h"
#include "UninstallOptions.h"
#include "PackageMatchFilter.h"
#include "PackageManagerSettings.h"
#include "DownloadOptions.h"
#include "AuthenticationArguments.h"
#include "RepairOptions.h"
#include "AddPackageCatalogOptions.h"
#include "RemovePackageCatalogOptions.h"
#pragma warning( pop )

namespace winrt::Microsoft::Management::Deployment
{
    CLSID GetRedirectedClsidFromInProcClsid(REFCLSID clsid)
    {
        if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_PackageManager))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::PackageManager);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_FindPackagesOptions))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::FindPackagesOptions);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_CreateCompositePackageCatalogOptions))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::CreateCompositePackageCatalogOptions);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_InstallOptions))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::InstallOptions);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_UninstallOptions))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::UninstallOptions);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_DownloadOptions))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::DownloadOptions);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_PackageMatchFilter))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::PackageMatchFilter);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_AuthenticationArguments))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::AuthenticationArguments);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_PackageManagerSettings))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::PackageManagerSettings);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_RepairOptions))
        {
           return __uuidof(winrt::Microsoft::Management::Deployment::implementation::RepairOptions);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_AddPackageCatalogOptions))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::AddPackageCatalogOptions);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_RemovePackageCatalogOptions))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::RemovePackageCatalogOptions);
        }
        else
        {
            return CLSID_NULL;
        }
    }
}
