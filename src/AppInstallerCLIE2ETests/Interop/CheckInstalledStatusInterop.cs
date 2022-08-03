// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;
    using System;
    using System.IO;
    using System.Threading.Tasks;

    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class CheckInstalledStatusInterop : BaseInterop
    {
        private string installDir;
        private PackageManager packageManager;
        private PackageCatalogReference testSource;

        public CheckInstalledStatusInterop(IInstanceInitializer initializer) : base(initializer) { }

        [SetUp]
        public void SetUp()
        {
            packageManager = TestFactory.CreatePackageManager();
            testSource = packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            installDir = TestCommon.GetRandomTestDir();

            // Find and install the test package.
            var searchResult = FindOnePackage(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.ReplacementInstallerArguments = $"/InstallDir {installDir} /ProductID CheckInstalledStatusProductId /DisplayName TestCheckInstalledStatus";
            var installResult = packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions).GetResults();
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Add the data file listed in the manifest
            File.WriteAllText(Path.Combine(installDir, "data.txt"), "Test");
        }

        [TearDown]
        public void Cleanup()
        {
            // Find and uninstall the test package.
            var options = TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = packageManager.CreateCompositePackageCatalog(options);
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            var uninstallOptions = TestFactory.CreateUninstallOptions();
            var uninstallResult = packageManager.UninstallPackageAsync(searchResult.CatalogPackage, uninstallOptions).GetResults();
            Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);

            // Delete the extra data file listed in the manifest
            File.Delete(Path.Combine(installDir, "data.txt"));
        }

        [Test]
        public void CheckInstalledStatus()
        {
            var searchResult = FindOnePackage(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            var checkResult = searchResult.CatalogPackage.CheckInstalledStatus();
            Assert.AreEqual(CheckInstalledStatusResultStatus.Ok, checkResult.Status);
            Assert.AreEqual(1, checkResult.PackageInstalledStatus.Count);

            var installerInstalledStatus = checkResult.PackageInstalledStatus[0];
            Assert.AreEqual(Windows.System.ProcessorArchitecture.Neutral, installerInstalledStatus.InstallerInfo.Architecture);
            Assert.AreEqual(PackageInstallerType.Exe, installerInstalledStatus.InstallerInfo.InstallerType);
            Assert.AreEqual(PackageInstallerType.Unknown, installerInstalledStatus.InstallerInfo.NestedInstallerType);
            Assert.AreEqual(PackageInstallerScope.Unknown, installerInstalledStatus.InstallerInfo.Scope);
        }
    }
}