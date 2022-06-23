// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using Microsoft.Management.Deployment;
    using NUnit.Framework;
    using System;
    using System.Linq;
    using System.Threading;
    using WinRT;

    public class COMInstallCommand : BaseCommand
    {
        PackageManager packageManager;
        PackageCatalog owcSource;
        FindPackagesOptions findPackageOptions;

        [SetUp]
        public void Init()
        {
            packageManager = new PackageManager();
            owcSource = packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog.OpenWindowsCatalog).Connect().PackageCatalog;
            findPackageOptions = new FindPackagesOptions();

            packageManager.GetPackageCatalogs();

            var templateCatalog = (IWinRTObject)packageManager.GetPackageCatalogByName("winget");
            var pcr = new PackageCatalogReference(templateCatalog.NativeObject);

            var ccpco = new CreateCompositePackageCatalogOptions();
            ccpco.Catalogs.Add(pcr);

            var test = packageManager.GetPackageCatalogs();

            var ccpc = packageManager.CreateCompositePackageCatalog(ccpco);
            
            var a = packageManager.GetPackageCatalogs();
        }

        [Test]
        public void InstallAppDoesNotExist()
        {
            findPackageOptions.Filters.Add(new()
            {
                Field = PackageMatchField.Id,
                Option = PackageFieldMatchOption.Equals,
                Value = "DoesNotExist"
            });

            var searchResult = owcSource.FindPackages(findPackageOptions).Matches;
            Assert.True(0 == searchResult.Count);
        }

        [Test]
        public void InstallWithMultipleAppsMatchingQuery()
        {
            findPackageOptions.Filters.Add(new()
            {
                Field = PackageMatchField.Id,
                Option = PackageFieldMatchOption.Equals,
                Value = "TestExeInstaller"
            });

            var searchResult = owcSource.FindPackages(findPackageOptions).Matches;
            Assert.True(0 == searchResult.Count);
        }
    }
}