// Lists all installed packages
using Microsoft.Management.Deployment;

var packageManager = new PackageManager();

var installedCatalogConnectResult = packageManager.GetLocalPackageCatalog(LocalPackageCatalog.InstalledPackages).Connect();
if (installedCatalogConnectResult.Status != ConnectResultStatus.Ok)
{
    throw new Exception("Error connecting to catalog");
}

var installedCatalog = installedCatalogConnectResult.PackageCatalog;

var findOptions = new FindPackagesOptions();
var searchResult = installedCatalog.FindPackages(findOptions);
if (searchResult.Status != FindPackagesResultStatus.Ok)
{
    throw new Exception("Error finding packages");
}

// Can't use foreach due to C#/WinRT limitations
for (int i = 0; i < searchResult.Matches.Count; ++i)
{
    Console.WriteLine("Package found: " + searchResult.Matches[i].CatalogPackage.Id);
}
