// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;

    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class FindPackagesInterop : BaseInterop
    {
        private PackageManager packageManager;
        private PackageCatalogReference testSource;

        public FindPackagesInterop(IInstanceInitializer initializer) : base(initializer) { }

        [SetUp]
        public void SetUp()
        {
            packageManager = TestFactory.CreatePackageManager();
            testSource = packageManager.GetPackageCatalogByName(Constants.TestSourceName);
        }

        [Test]
        public void FindPackageDoesNotExist()
        {
            // Find package
            var searchResult = FindAllPackages(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "DoesNotExist");
            
            // Assert
            Assert.AreEqual(0, searchResult.Count);
        }

        [Test]
        public void FindPackagesMultipleMatchingQuery()
        {
            // Find package
            var searchResult = FindAllPackages(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestExeInstaller");
            
            // Assert
            Assert.AreEqual(2, searchResult.Count);
        }
    }
}