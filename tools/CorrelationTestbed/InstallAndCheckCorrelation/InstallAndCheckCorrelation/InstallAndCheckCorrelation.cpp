// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <Windows.h>

#include <wil/cppwinrt.h>
#include <wil/result.h>
#include <wil/safecast.h>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.Management.Deployment.h>

#include <iostream>
#include <filesystem>
#include <fstream>

using namespace std::string_view_literals;
using namespace winrt::Microsoft::Management::Deployment;
using namespace winrt::Windows::Foundation;

template <typename T>
struct JSONPair
{
    JSONPair(std::string_view name, const T& value, bool comma = true) :
        Name(name), Value(value), Comma(comma)
    {}

    std::string_view Name;
    const T& Value;
    bool Comma;
};

template <typename T>
struct JSONQuote
{
    constexpr static bool value = true;
};

template <>
struct JSONQuote<HRESULT>
{
    constexpr static bool value = false;
};

template <>
struct JSONQuote<bool>
{
    constexpr static bool value = false;
};

template <typename T>
std::ostream& operator<<(std::ostream& out, const JSONPair<T>& pair)
{
    out << '"' << pair.Name << "\": ";
    if (JSONQuote<T>::value)
    {
        out << '"';
    }
    out << pair.Value;
    if (JSONQuote<T>::value)
    {
        out << '"';
    }
    if (pair.Comma)
    {
        out << ',';
    }
    return out << std::endl;
}

std::string ConvertToUTF8(std::wstring_view input)
{
    if (input.empty())
    {
        return {};
    }

    int utf8ByteCount = WideCharToMultiByte(CP_UTF8, 0, input.data(), wil::safe_cast<int>(input.length()), nullptr, 0, nullptr, nullptr);
    THROW_LAST_ERROR_IF(utf8ByteCount == 0);

    // Since the string view should not contain the null char, the result won't either.
    // This allows us to use the resulting size value directly in the string constructor.
    std::string result(wil::safe_cast<size_t>(utf8ByteCount), '\0');

    int utf8BytesWritten = WideCharToMultiByte(CP_UTF8, 0, input.data(), wil::safe_cast<int>(input.length()), &result[0], wil::safe_cast<int>(result.size()), nullptr, nullptr);
    FAIL_FAST_HR_IF(E_UNEXPECTED, utf8ByteCount != utf8BytesWritten);

    return result;
}

std::wstring ConvertToUTF16(std::string_view input, UINT codePage = CP_UTF8)
{
    if (input.empty())
    {
        return {};
    }

    int utf16CharCount = MultiByteToWideChar(codePage, 0, input.data(), wil::safe_cast<int>(input.length()), nullptr, 0);
    THROW_LAST_ERROR_IF(utf16CharCount == 0);

    // Since the string view should not contain the null char, the result won't either.
    // This allows us to use the resulting size value directly in the string constructor.
    std::wstring result(wil::safe_cast<size_t>(utf16CharCount), L'\0');

    int utf16CharsWritten = MultiByteToWideChar(codePage, 0, input.data(), wil::safe_cast<int>(input.length()), &result[0], wil::safe_cast<int>(result.size()));
    FAIL_FAST_HR_IF(E_UNEXPECTED, utf16CharCount != utf16CharsWritten);

    return result;
}

// CLSIDs for WinGet package
const CLSID CLSID_PackageManager = { 0xC53A4F16, 0x787E, 0x42A4, 0xB3, 0x04, 0x29, 0xEF, 0xFB, 0x4B, 0xF5, 0x97 };  //C53A4F16-787E-42A4-B304-29EFFB4BF597
const CLSID CLSID_InstallOptions = { 0x1095f097, 0xEB96, 0x453B, 0xB4, 0xE6, 0x16, 0x13, 0x63, 0x7F, 0x3B, 0x14 };  //1095F097-EB96-453B-B4E6-1613637F3B14
const CLSID CLSID_FindPackagesOptions = { 0x572DED96, 0x9C60, 0x4526, { 0x8F, 0x92, 0xEE, 0x7D, 0x91, 0xD3, 0x8C, 0x1A } }; //572DED96-9C60-4526-8F92-EE7D91D38C1A
const CLSID CLSID_PackageMatchFilter = { 0xD02C9DAF, 0x99DC, 0x429C, { 0xB5, 0x03, 0x4E, 0x50, 0x4E, 0x4A, 0xB0, 0x00 } }; //D02C9DAF-99DC-429C-B503-4E504E4AB000
const CLSID CLSID_CreateCompositePackageCatalogOptions = { 0x526534B8, 0x7E46, 0x47C8, { 0x84, 0x16, 0xB1, 0x68, 0x5C, 0x32, 0x7D, 0x37 } }; //526534B8-7E46-47C8-8416-B1685C327D37

// CLSIDs for WinGetDev package
const CLSID CLSID_PackageManager2 = { 0x74CB3139, 0xB7C5, 0x4B9E, { 0x93, 0x88, 0xE6, 0x61, 0x6D, 0xEA, 0x28, 0x8C } };  //74CB3139-B7C5-4B9E-9388-E6616DEA288C
const CLSID CLSID_InstallOptions2 = { 0x44FE0580, 0x62F7, 0x44D4, 0x9E, 0x91, 0xAA, 0x96, 0x14, 0xAB, 0x3E, 0x86 };  //44FE0580-62F7-44D4-9E91-AA9614AB3E86
const CLSID CLSID_FindPackagesOptions2 = { 0x1BD8FF3A, 0xEC50, 0x4F69, { 0xAE, 0xEE, 0xDF, 0x4C, 0x9D, 0x3B, 0xAA, 0x96 } }; //1BD8FF3A-EC50-4F69-AEEE-DF4C9D3BAA96
const CLSID CLSID_PackageMatchFilter2 = { 0x3F85B9F4, 0x487A, 0x4C48, { 0x90, 0x35, 0x29, 0x03, 0xF8, 0xA6, 0xD9, 0xE8 } }; //3F85B9F4-487A-4C48-9035-2903F8A6D9E8
const CLSID CLSID_CreateCompositePackageCatalogOptions2 = { 0xEE160901, 0xB317, 0x4EA7, { 0x9C, 0xC6, 0x53, 0x55, 0xC6, 0xD7, 0xD8, 0xA7 } }; //EE160901-B317-4EA7-9CC6-5355C6D7D8A7

// Helper object to make cleaner error handling
struct Main
{
    std::string packageIdentifier;
    std::string sourceName;
    std::filesystem::path outputPath;
    bool useDevCLSIDs = false;
    bool onlyCorrelate = false;

    int ParseArgs(int argc, char** argv)
    {
        // Supports the following arguments:
        //  -id : [Required] The PackageIdentifier to install and check for correlation
        //  -src : [Required] The source name for the package to install
        //  -out : [Required] The file to write results to
        //  -dev : [Optional] Use the dev CLSIDs
        //  -cor : [Optional] Only correlate the package

        for (int i = 1; i < argc; ++i)
        {
            if ("-id"sv == argv[i] && i + 1 < argc)
            {
                packageIdentifier = argv[++i];
            }
            else if ("-src"sv == argv[i] && i + 1 < argc)
            {
                sourceName = argv[++i];
            }
            else if ("-out"sv == argv[i] && i + 1 < argc)
            {
                outputPath = argv[++i];
            }
            else if ("-dev"sv == argv[i])
            {
                useDevCLSIDs = true;
            }
            else if ("-cor"sv == argv[i])
            {
                onlyCorrelate = true;
            }
        }

        // Check inputs
        if (outputPath.empty())
        {
            std::cout << "No output file path specified, use -out" << std::endl;
            return 2;
        }

        if (!outputPath.has_stem())
        {
            std::cout << "Output path is not a file" << std::endl;
            return 3;
        }

        std::filesystem::create_directories(outputPath.parent_path());

        outputStream.open(outputPath);

        if (!outputStream)
        {
            std::cout << "Output file could not be created" << std::endl;
            return 4;
        }

        return 0;
    }

    PackageManager CreatePackageManager()
    {
        if (useDevCLSIDs)
        {
            return winrt::create_instance<PackageManager>(CLSID_PackageManager2, CLSCTX_ALL);
        }
        return winrt::create_instance<PackageManager>(CLSID_PackageManager, CLSCTX_ALL);
    }

    InstallOptions CreateInstallOptions()
    {
        if (useDevCLSIDs)
        {
            return winrt::create_instance<InstallOptions>(CLSID_InstallOptions2, CLSCTX_ALL);
        }
        return winrt::create_instance<InstallOptions>(CLSID_InstallOptions, CLSCTX_ALL);
    }

    FindPackagesOptions CreateFindPackagesOptions()
    {
        if (useDevCLSIDs)
        {
            return winrt::create_instance<FindPackagesOptions>(CLSID_FindPackagesOptions2, CLSCTX_ALL);
        }
        return winrt::create_instance<FindPackagesOptions>(CLSID_FindPackagesOptions, CLSCTX_ALL);
    }

    CreateCompositePackageCatalogOptions CreateCreateCompositePackageCatalogOptions()
    {
        if (useDevCLSIDs)
        {
            return winrt::create_instance<CreateCompositePackageCatalogOptions>(CLSID_CreateCompositePackageCatalogOptions2, CLSCTX_ALL);
        }
        return winrt::create_instance<CreateCompositePackageCatalogOptions>(CLSID_CreateCompositePackageCatalogOptions, CLSCTX_ALL);
    }

    PackageMatchFilter CreatePackageMatchFilter()
    {
        if (useDevCLSIDs)
        {
            return winrt::create_instance<PackageMatchFilter>(CLSID_PackageMatchFilter2, CLSCTX_ALL);
        }
        return winrt::create_instance<PackageMatchFilter>(CLSID_PackageMatchFilter, CLSCTX_ALL);
    }

    std::ofstream outputStream;

    // Result file outputs
    HRESULT hr = S_OK;
    std::string error;
    std::string phase;
    std::string action;
    std::string packageName;
    std::string packagePublisher;
    bool correlatePackageKnown = false;
    std::string packageKnownName;
    std::string packageKnownPublisher;
    bool correlateArchive = false;
    std::string archiveName;
    std::string archivePublisher;

    void ValidateArgs()
    {
        if (packageIdentifier.empty())
        {
            hr = E_INVALIDARG;
            error = "A package identifier must be supplied, use -id";
            return;
        }

        if (sourceName.empty())
        {
            hr = E_INVALIDARG;
            error = "A source name must be supplied, use -src";
            return;
        }
    }

    void Install()
    {
        try
        {
            action = "Create package manager";
            auto packageManager = CreatePackageManager();

            action = "Get source reference";
            auto sourceRef = packageManager.GetPackageCatalogByName(ConvertToUTF16(sourceName));

            action = "Connecting to catalog";
            auto connectResult = sourceRef.Connect();

            if (connectResult.Status() != ConnectResultStatus::Ok)
            {
                hr = E_FAIL;
                error = "Error connecting to catalog";
                return;
            }

            auto catalog = connectResult.PackageCatalog();

            action = "Create find options";
            auto findOptions = CreateFindPackagesOptions();

            action = "Add package id filter";
            auto filter = CreatePackageMatchFilter();
            filter.Field(PackageMatchField::Id);
            filter.Option(PackageFieldMatchOption::Equals);
            filter.Value(ConvertToUTF16(packageIdentifier));
            findOptions.Filters().Append(filter);

            action = "Find package";
            auto findResult = catalog.FindPackages(findOptions);

            if (findResult.Status() != FindPackagesResultStatus::Ok)
            {
                hr = E_FAIL;
                error = "Error finding packages";
                return;
            }

            action = "Get match";
            auto matches = findResult.Matches();

            if (matches.Size() == 0)
            {
                hr = E_NOT_SET;
                error = "Package not found";
                return;
            }

            auto package = matches.GetAt(0).CatalogPackage();

            action = "Inspect package";
            auto installVersion = package.DefaultInstallVersion();
            packageName = ConvertToUTF8(installVersion.DisplayName());
            if (useDevCLSIDs)
            {
                // Publisher is not yet available on the release version; make this unconditional when it is
                packagePublisher = ConvertToUTF8(installVersion.Publisher());
            }

            if (!onlyCorrelate)
            {
                action = "Create install options";
                auto installOptions = CreateInstallOptions();

                installOptions.PackageInstallScope(PackageInstallScope::Any);
                installOptions.PackageInstallMode(PackageInstallMode::Silent);

                std::cout << "Beginning to install " << packageIdentifier << " (" << packageName << ") from " << sourceName << "..." << std::endl;
                auto installOperation = packageManager.InstallPackageAsync(package, installOptions);

                if (installOperation.wait_for(std::chrono::minutes(10)) != AsyncStatus::Completed)
                {
                    hr = E_FAIL;
                    error = "Install operation timed out";
                    return;
                }

                auto installResult = installOperation.GetResults();
                if (installResult.Status() != InstallResultStatus::Ok)
                {
                    hr = installResult.ExtendedErrorCode();
                    error = "Error installing package";
                    return;
                }
            }
        }
        catch (const winrt::hresult_error& hre)
        {
            hr = hre.code();
            error = ConvertToUTF8(hre.message());
        }
    }

    void CorrelatePackageKnown()
    {
        try
        {
            action = "Create package manager";
            auto packageManager = CreatePackageManager();

            action = "Get source reference";
            auto sourceRef = packageManager.GetPackageCatalogByName(ConvertToUTF16(sourceName));

            action = "Create composite catalog options";
            auto compOptions = CreateCreateCompositePackageCatalogOptions();

            compOptions.Catalogs().Append(sourceRef);
            compOptions.CompositeSearchBehavior(CompositeSearchBehavior::RemotePackagesFromAllCatalogs);

            action = "Create composite catalog reference";
            auto compRef = packageManager.CreateCompositePackageCatalog(compOptions);

            action = "Connecting to catalog";
            auto connectResult = compRef.Connect();

            if (connectResult.Status() != ConnectResultStatus::Ok)
            {
                hr = E_FAIL;
                error = "Error connecting to catalog";
                return;
            }

            auto catalog = connectResult.PackageCatalog();

            action = "Create find options";
            auto findOptions = CreateFindPackagesOptions();

            action = "Add package id filter";
            auto filter = CreatePackageMatchFilter();
            filter.Field(PackageMatchField::Id);
            filter.Option(PackageFieldMatchOption::Equals);
            filter.Value(ConvertToUTF16(packageIdentifier));
            findOptions.Filters().Append(filter);

            action = "Find package";
            auto findResult = catalog.FindPackages(findOptions);

            if (findResult.Status() != FindPackagesResultStatus::Ok)
            {
                hr = E_FAIL;
                error = "Error finding packages";
                return;
            }

            action = "Get match";
            auto matches = findResult.Matches();

            if (matches.Size() == 0)
            {
                hr = E_NOT_SET;
                error = "Package not found";
                return;
            }

            auto package = matches.GetAt(0).CatalogPackage();

            action = "Inspect package for installed version";
            auto installed = package.InstalledVersion();

            if (installed)
            {
                correlatePackageKnown = true;
                packageKnownName = ConvertToUTF8(installed.DisplayName());
                if (useDevCLSIDs)
                {
                    // Publisher is not yet available on the release version; make this unconditional when it is
                    packageKnownPublisher = ConvertToUTF8(installed.Publisher());
                }
            }
        }
        catch (const winrt::hresult_error& hre)
        {
            hr = hre.code();
            error = ConvertToUTF8(hre.message());
        }
    }

    void CorrelateArchive()
    {
        try
        {
            action = "Create package manager";
            auto packageManager = CreatePackageManager();

            action = "Get source reference";
            auto sourceRef = packageManager.GetPackageCatalogByName(ConvertToUTF16(sourceName));

            action = "Create composite catalog options";
            auto compOptions = CreateCreateCompositePackageCatalogOptions();

            compOptions.Catalogs().Append(sourceRef);
            compOptions.CompositeSearchBehavior(CompositeSearchBehavior::LocalCatalogs);

            action = "Create composite catalog reference";
            auto compRef = packageManager.CreateCompositePackageCatalog(compOptions);

            action = "Connecting to catalog";
            auto connectResult = compRef.Connect();

            if (connectResult.Status() != ConnectResultStatus::Ok)
            {
                hr = E_FAIL;
                error = "Error connecting to catalog";
                return;
            }

            auto catalog = connectResult.PackageCatalog();

            action = "Create find options";
            auto findOptions = CreateFindPackagesOptions();

            action = "Find package";
            auto findResult = catalog.FindPackages(findOptions);

            if (findResult.Status() != FindPackagesResultStatus::Ok)
            {
                hr = E_FAIL;
                error = "Error finding packages";
                return;
            }

            action = "Get matches";
            auto matches = findResult.Matches();

            action = "Get source info";
            auto sourceInfo = sourceRef.Info();
            auto sourceIdentifier = sourceInfo.Id();
            auto sourceType = sourceInfo.Type();

            for (const auto& match : matches)
            {
                auto package = match.CatalogPackage();

                if (ConvertToUTF8(package.Id()) != packageIdentifier)
                {
                    continue;
                }

                auto installed = package.InstalledVersion();

                if (installed)
                {
                    auto installedCatalogInfo = installed.PackageCatalog().Info();

                    if (installedCatalogInfo.Id() == sourceIdentifier && installedCatalogInfo.Type() == sourceType)
                    {
                        correlateArchive = true;
                        archiveName = ConvertToUTF8(installed.DisplayName());
                        if (useDevCLSIDs)
                        {
                            // Publisher is not yet available on the release version; make this unconditional when it is
                            archivePublisher = ConvertToUTF8(installed.Publisher());
                        }
                        break;
                    }
                }
            }
        }
        catch (const winrt::hresult_error& hre)
        {
            hr = hre.code();
            error = ConvertToUTF8(hre.message());
        }
    }

    void ReportResult()
    {
        if (outputStream)
        {
            outputStream << "{" << std::endl;
            outputStream << JSONPair{ "PackageIdentifier", packageIdentifier };
            outputStream << JSONPair{ "Source", sourceName };
            outputStream << JSONPair{ "UseDev", useDevCLSIDs };
            outputStream << JSONPair{ "Error", error };
            outputStream << JSONPair{ "Phase", phase };
            outputStream << JSONPair{ "Action", action };
            outputStream << JSONPair{ "PackageName", packageName };
            outputStream << JSONPair{ "PackagePublisher", packagePublisher };
            outputStream << JSONPair{ "CorrelatePackageKnown", correlatePackageKnown };
            outputStream << JSONPair{ "PackageKnownName", packageKnownName };
            outputStream << JSONPair{ "PackageKnownPublisher", packageKnownPublisher };
            outputStream << JSONPair{ "CorrelateArchive", correlateArchive };
            outputStream << JSONPair{ "ArchiveName", archiveName };
            outputStream << JSONPair{ "ArchivePublisher", archivePublisher };
            // Keep at the end to prevent a dangling comma
            outputStream << JSONPair{ "HRESULT", hr, false } << "}" << std::endl;
        }
    }

    void main(int argc, char** argv)
    {
        hr = ParseArgs(argc, argv);
        if (hr != 0)
        {
            return;
        }

        ValidateArgs();
        if (FAILED(hr))
        {
            return;
        }

        auto co_uninitialize = wil::CoInitializeEx();

        // Execute the install step
        phase = "Install";
        std::cout << "Connecting to PackageManager..." << std::endl;
        Install();
        if (FAILED(hr))
        {
            return;
        }

        // Check for the installed package being correlated when the remote package is known,
        // as when trying to determine information about a single known package.
        phase = "Correlate when package known";
        std::cout << "Correlating package when known..." << std::endl;
        CorrelatePackageKnown();

        // Check for the installed package being correlated when archiving local package information.
        phase = "Correlate when archiving";
        std::cout << "Correlating package when archiving..." << std::endl;
        CorrelateArchive();

        std::cout << "Done" << std::endl;
        phase = "Completed";
        action.clear();
    }
};

int main(int argc, char** argv) try
{
    Main mainMain;
    mainMain.main(argc, argv);
    mainMain.ReportResult();
    return mainMain.hr;
}
catch (const std::exception& e)
{
    std::cout << "Exception occurred: " << e.what() << std::endl;
    return 1;
}
catch (...)
{
    std::cout << "Unknown exception occurred" << std::endl;
    return 1;
}
