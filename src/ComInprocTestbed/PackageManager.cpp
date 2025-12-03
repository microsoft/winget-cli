// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageManager.h"

using namespace winrt::Microsoft::Management::Deployment;

PackageCatalog Connect(const PackageCatalogReference& reference, std::string_view name)
{
    auto connectResult = reference.Connect();

    if (connectResult.Status() != ConnectResultStatus::Ok)
    {
        std::cout << "Connecting to " << name << " got: " << static_cast<int32_t>(connectResult.Status()) << " [" << connectResult.ExtendedErrorCode() << "]\n";
        return nullptr;
    }

    return connectResult.PackageCatalog();
}

template <typename Operation>
auto WaitForResult(Operation&& operation)
{
    std::promise<void> promise;
    auto future = promise.get_future();
    operation.Completed([&](const auto&, const auto&) { promise.set_value(); });
    future.wait();
    return operation.GetResults();
}

bool UsePackageManager(const TestParameters& testParameters)
{
    PackageManager packageManager = testParameters.CreatePackageManager();

    // Force installed cache to be created
    auto installedCatalogRef = packageManager.GetLocalPackageCatalog(LocalPackageCatalog::InstalledPackages);
    auto installedCatalog = Connect(installedCatalogRef, "Installed Catalog");
    if (!installedCatalog)
    {
        return false;
    }

    // Force TerminationSignalHandler to be created
    CreateCompositePackageCatalogOptions options = testParameters.CreateCreateCompositePackageCatalogOptions();
    options.CompositeSearchBehavior(CompositeSearchBehavior::RemotePackagesFromRemoteCatalogs);

    auto sourceName = winrt::to_hstring(testParameters.SourceName);

    for (PackageCatalogReference catalogRef : packageManager.GetPackageCatalogs())
    {
        if (sourceName.empty() || catalogRef.Info().Name() == sourceName)
        {
            options.Catalogs().Append(catalogRef);
        }
    }

    // Inputs are provided for a source that we did not find; add it.
    if (!sourceName.empty() && !testParameters.SourceURL.empty() && options.Catalogs().Size() == 0)
    {
        AddPackageCatalogOptions addPackageCatalogOptions = testParameters.CreateAddPackageCatalogOptions();
        addPackageCatalogOptions.Name(sourceName);
        addPackageCatalogOptions.SourceUri(winrt::to_hstring(testParameters.SourceURL));
        addPackageCatalogOptions.TrustLevel(PackageCatalogTrustLevel::Trusted);

        auto addCatalogResult = WaitForResult(packageManager.AddPackageCatalogAsync(addPackageCatalogOptions));

        if (addCatalogResult.Status() != AddPackageCatalogStatus::Ok)
        {
            std::cout << "Adding catalog `" << testParameters.SourceName << "` [`" << testParameters.SourceURL << "`] got: " << static_cast<int32_t>(addCatalogResult.Status()) << " [" << addCatalogResult.ExtendedErrorCode() << "]\n";
            return false;
        }

        // Get the new catalog
        options.Catalogs().Append(packageManager.GetPackageCatalogByName(sourceName));
    }

    auto compositeCatalog = Connect(packageManager.CreateCompositePackageCatalog(options), "Composite Catalog");
    if (!compositeCatalog)
    {
        return false;
    }

    PackageMatchFilter filter = testParameters.CreatePackageMatchFilter();
    filter.Field(PackageMatchField::Id);
    filter.Option(PackageFieldMatchOption::EqualsCaseInsensitive);
    filter.Value(winrt::to_hstring(testParameters.PackageName));

    FindPackagesOptions findOptions = testParameters.CreateFindPackagesOptions();
    findOptions.Filters().Append(filter);

    auto findResult = compositeCatalog.FindPackages(findOptions);
    if (findResult.Status() != FindPackagesResultStatus::Ok)
    {
        std::cout << "Finding package " << testParameters.PackageName << " got: " << static_cast<int32_t>(findResult.Status()) << " [" << findResult.ExtendedErrorCode() << "]\n";
        return false;
    }

    if (findResult.Matches().Size() != 1)
    {
        std::cout << "Finding package " << testParameters.PackageName << " got " << findResult.Matches().Size() << " results.\n";
        return false;
    }

    DownloadOptions downloadOptions = testParameters.CreateDownloadOptions();
    auto downloadResult = WaitForResult(packageManager.DownloadPackageAsync(findResult.Matches().GetAt(0).CatalogPackage(), downloadOptions));

    if (downloadResult.Status() != DownloadResultStatus::Ok)
    {
        std::cout << "Downloading package " << testParameters.PackageName << " got: " << static_cast<int32_t>(downloadResult.Status()) << "\n";
        return false;
    }

    return true;
}

void InitializePackageManagerGlobals()
{
    PackageManagerSettings settings;
    settings.SetCallerIdentifier(L"ComInprocTestbed");
    settings.SetStateIdentifier(L"ComInprocTestbed");
}

void SetUnloadPreference(bool value)
{
    PackageManagerSettings settings;
    settings.CanUnloadPreference(value);
}
