#include <Windows.h>

#include <winrt/Microsoft.Management.Deployment.h>
#include <winrt/Windows.Foundation.Collections.h>

#include <iostream>

using namespace winrt::Microsoft::Management::Deployment;

const CLSID CLSID_PackageManager = { 0xC53A4F16, 0x787E, 0x42A4, 0xB3, 0x04, 0x29, 0xEF, 0xFB, 0x4B, 0xF5, 0x97 };  //C53A4F16-787E-42A4-B304-29EFFB4BF597
const CLSID CLSID_FindPackagesOptions = { 0x572DED96, 0x9C60, 0x4526, { 0x8F, 0x92, 0xEE, 0x7D, 0x91, 0xD3, 0x8C, 0x1A } }; //572DED96-9C60-4526-8F92-EE7D91D38C1A

// Simple RAII for COM initialization
struct ComInitialization
{
    ComInitialization()
    {
        winrt::check_hresult(CoInitialize(nullptr));
    }

    ~ComInitialization()
    {
        CoUninitialize();
    }
};

// Lists all installed packages
int main()
{
    ComInitialization comInitialization;

    auto packageManager = winrt::create_instance<PackageManager>(CLSID_PackageManager, CLSCTX_ALL);

    auto installedCatalogConnectResult = packageManager.GetLocalPackageCatalog(LocalPackageCatalog::InstalledPackages).Connect();
    if (installedCatalogConnectResult.Status() != ConnectResultStatus::Ok)
    {
        std::wcerr << L"Error connecting to catalog" << std::endl;
        return E_FAIL;
    }
    auto installedCatalog = installedCatalogConnectResult.PackageCatalog();

    auto findOptions = winrt::create_instance<FindPackagesOptions>(CLSID_FindPackagesOptions, CLSCTX_ALL);
    auto searchResult = installedCatalog.FindPackages(findOptions);
    if (searchResult.Status() != FindPackagesResultStatus::Ok)
    {
        std::wcerr << L"Error finding packages" << std::endl;
        return E_FAIL;
    }

    for (auto package : searchResult.Matches())
    {
        std::wcout << L"Found package: " << package.CatalogPackage().Id().c_str() << std::endl;
    }
}
