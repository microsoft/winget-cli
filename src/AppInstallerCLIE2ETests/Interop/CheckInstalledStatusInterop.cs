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
        }

        [Test]
        public async Task CheckInstalledStatus()
        {
            // Find and install the test package.
            var searchResult = FindOnePackage(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.ReplacementInstallerArguments = $"/InstallDir {installDir} /ProductID CheckInstalledStatusProductId /DisplayName TestCheckInstalledStatus";
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            var checkResult = searchResult.CatalogPackage.CheckInstalledStatus();
            Assert.AreEqual(CheckInstalledStatusResultStatus.Ok, checkResult.Status);
            Assert.AreEqual(1, checkResult.PackageInstalledStatus.Count);

            // checkResult.InstalledStatus[0].
        }
    }
}