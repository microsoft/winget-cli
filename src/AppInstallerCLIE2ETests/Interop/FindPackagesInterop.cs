// -----------------------------------------------------------------------------
// <copyright file="FindPackagesInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;

    /// <summary>
    /// Test find package interop.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class FindPackagesInterop : BaseInterop
    {
        private PackageManager packageManager;
        private PackageCatalogReference testSource;

        /// <summary>
        /// Initializes a new instance of the <see cref="FindPackagesInterop"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public FindPackagesInterop(IInstanceInitializer initializer)
            : base(initializer)
        {
        }

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void SetUp()
        {
            this.packageManager = this.TestFactory.CreatePackageManager();
            this.testSource = this.packageManager.GetPackageCatalogByName(Constants.TestSourceName);
        }

        /// <summary>
        /// Test find package. no package.
        /// </summary>
        [Test]
        public void FindPackageDoesNotExist()
        {
            // Find package
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "DoesNotExist");

            // Assert
            Assert.AreEqual(0, searchResult.Count);
        }

        /// <summary>
        /// Test find package with multiple match.
        /// </summary>
        [Test]
        public void FindPackagesMultipleMatchingQuery()
        {
            // Find package
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestExeInstaller");

            // Assert
            Assert.AreEqual(2, searchResult.Count);
        }
    }
}