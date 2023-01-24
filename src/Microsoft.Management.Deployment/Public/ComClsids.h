// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <guiddef.h>

// Clsids for out-of-proc com invocation
#if USE_PROD_CLSIDS 
#define WINGET_OUTOFPROC_COM_CLSID_PackageManager "C53A4F16-787E-42A4-B304-29EFFB4BF597"
#define WINGET_OUTOFPROC_COM_CLSID_FindPackagesOptions "572DED96-9C60-4526-8F92-EE7D91D38C1A"
#define WINGET_OUTOFPROC_COM_CLSID_CreateCompositePackageCatalogOptions "526534B8-7E46-47C8-8416-B1685C327D37"
#define WINGET_OUTOFPROC_COM_CLSID_InstallOptions "1095F097-EB96-453B-B4E6-1613637F3B14"
#define WINGET_OUTOFPROC_COM_CLSID_UninstallOptions "E1D9A11E-9F85-4D87-9C17-2B93143ADB8D"
#define WINGET_OUTOFPROC_COM_CLSID_PackageMatchFilter "D02C9DAF-99DC-429C-B503-4E504E4AB000"
#define WINGET_OUTOFPROC_COM_CLSID_ConnectOptions "B5033698-79D1-4B94-9C39-0EC4EF1C7853"
#else
#define WINGET_OUTOFPROC_COM_CLSID_PackageManager "74CB3139-B7C5-4B9E-9388-E6616DEA288C"
#define WINGET_OUTOFPROC_COM_CLSID_FindPackagesOptions "1BD8FF3A-EC50-4F69-AEEE-DF4C9D3BAA96"
#define WINGET_OUTOFPROC_COM_CLSID_CreateCompositePackageCatalogOptions "EE160901-B317-4EA7-9CC6-5355C6D7D8A7"
#define WINGET_OUTOFPROC_COM_CLSID_InstallOptions "44FE0580-62F7-44D4-9E91-AA9614AB3E86"
#define WINGET_OUTOFPROC_COM_CLSID_UninstallOptions "AA2A5C04-1AD9-46C4-B74F-6B334AD7EB8C"
#define WINGET_OUTOFPROC_COM_CLSID_PackageMatchFilter "3F85B9F4-487A-4C48-9035-2903F8A6D9E8"
#define WINGET_OUTOFPROC_COM_CLSID_ConnectOptions "6C4F68AC-F601-42FC-8CAF-87D3B3321783"
#endif

// Clsids only used in in-proc invocation
#define WINGET_INPROC_ONLY_COM_CLSID_PackageManagerSettings "80CF9D63-5505-4342-B9B4-BB87895CA8BB"

namespace winrt::Microsoft::Management::Deployment
{
    // clsid constants for in-proc com invocation
    const CLSID WINGET_INPROC_COM_CLSID_PackageManager = { 0x2DDE4456, 0x64D9, 0x4673, 0x8F, 0x7E, 0xA4, 0xF1, 0x9A, 0x2E, 0x6C, 0xC3 }; // 2DDE4456-64D9-4673-8F7E-A4F19A2E6CC3
    const CLSID WINGET_INPROC_COM_CLSID_FindPackagesOptions = { 0x96B9A53A, 0x9228, 0x4DA0, 0xB0, 0x13, 0xBB, 0x1B, 0x20, 0x31, 0xAB, 0x3D }; // 96B9A53A-9228-4DA0-B013-BB1B2031AB3D
    const CLSID WINGET_INPROC_COM_CLSID_CreateCompositePackageCatalogOptions = { 0x768318A6, 0x2EB5, 0x400D, 0x84, 0xD0, 0xDF, 0x35, 0x34, 0xC3, 0x0F, 0x5D }; // 768318A6-2EB5-400D-84D0-DF3534C30F5D
    const CLSID WINGET_INPROC_COM_CLSID_InstallOptions = { 0xE2AF3BA8, 0x8A88, 0x4766, 0x9D, 0xDA, 0xAE, 0x40, 0x13, 0xAD, 0xE2, 0x86 }; // E2AF3BA8-8A88-4766-9DDA-AE4013ADE286
    const CLSID WINGET_INPROC_COM_CLSID_UninstallOptions = { 0x869CB959, 0xEB54, 0x425C, 0xA1, 0xE4, 0x1A, 0x1C, 0x29, 0x1C, 0x64, 0xE9 }; // 869CB959-EB54-425C-A1E4-1A1C291C64E9
    const CLSID WINGET_INPROC_COM_CLSID_PackageMatchFilter = { 0x57DC8962, 0x7343, 0x42CD, 0xB9, 0x1C, 0x04, 0xF6, 0xA2, 0x5D, 0xB1, 0xD0 }; // 57DC8962-7343-42CD-B91C-04F6A25DB1D0
    const CLSID WINGET_INPROC_COM_CLSID_PackageManagerSettings = { 0x80CF9D63, 0x5505, 0x4342, 0xB9, 0xB4, 0xBB, 0x87, 0x89, 0x5C, 0xA8, 0xBB }; // 80CF9D63-5505-4342-B9B4-BB87895CA8BB
    const CLSID WINGET_INPROC_COM_CLSID_ConnectOptions = { 0xD026FDDC, 0x44D3, 0x443A, 0x8D, 0xAB, 0xA4, 0xDD, 0x96, 0x99, 0x43, 0xB3 }; //D026FDDC-44D3-443A-8DAB-A4DD969943B3

    CLSID GetRedirectedClsidFromInProcClsid(REFCLSID clsid);
}