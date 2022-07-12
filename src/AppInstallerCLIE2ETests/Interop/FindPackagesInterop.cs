// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;
    using System.Diagnostics;

    [TestFixtureSource(typeof(InitializersSource), nameof(InitializersSource.InProcess), Category = nameof(InitializersSource.InProcess))]
    [TestFixtureSource(typeof(InitializersSource), nameof(InitializersSource.OutOfProcess), Category = nameof(InitializersSource.OutOfProcess))]
    public class FindPackagesInterop : BaseInterop
    {
        private PackageManager packageManager;
        private PackageCatalogReference testSource;

        public FindPackagesInterop(IInstanceInitializer initializer) : base(initializer) { }

        [SetUp]
        public void SetUp()
        {
            var processFileName = Process.GetCurrentProcess().MainModule.FileName;

            // Print exe path
            Assert.Fail(processFileName, "Test runner path");
            packageManager = TestFactory.CreatePackageManager();
            testSource = packageManager.GetPackageCatalogByName(Constants.TestSourceName);
        }

        [Test]
        public void PackageDoesNotExist()
        {
            // Find package
            var searchResult = FindAllPackages(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "DoesNotExist");
            
            // Assert
            Assert.AreEqual(0, searchResult.Count);
        }

        [Test]
        public void MultiplePackagesMatchingQuery()
        {
            // Find package
            var searchResult = FindAllPackages(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestExeInstaller");
            
            // Assert
            Assert.AreEqual(2, searchResult.Count);
        }
    }
}