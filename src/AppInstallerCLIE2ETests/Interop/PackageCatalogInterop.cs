// -----------------------------------------------------------------------------
// <copyright file="PackageCatalogInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.CodeAnalysis;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;
    using Windows.System;

    /// <summary>
    /// Package catalog interop class.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class PackageCatalogInterop : BaseInterop
    {
        private PackageManager packageManager;

        /// <summary>
        /// Initializes a new instance of the <see cref="PackageCatalogInterop"/> class.
        /// </summary>
        /// <param name="initializer">initializer.</param>
        public PackageCatalogInterop(IInstanceInitializer initializer)
            : base(initializer)
        {
        }

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            // Remove the tests source if it exists.
            TestCommon.RunAICLICommand("source remove", Constants.TestSourceName);

            this.packageManager = this.TestFactory.CreatePackageManager();
        }

        /// <summary>
        /// Add and remove package catalog.
        /// </summary>
        /// <returns> representing the asynchronous unit test.</returns>
        [Test]
        public async Task AddRemovePackageCatalog()
        {
            AddPackageCatalogOptions options = this.TestFactory.CreateAddPackageCatalogOptions();
            options.SourceUri = Constants.TestSourceUrl;
            options.Name = Constants.TestSourceName;
            options.TrustLevel = PackageCatalogTrustLevel.Trusted;

            await this.AddAndValidatePackageCatalogAsync(options, AddPackageCatalogStatus.Ok);

            RemovePackageCatalogOptions removePackageCatalogOptions = this.TestFactory.CreateRemovePackageCatalogOptions();
            removePackageCatalogOptions.Name = Constants.TestSourceName;
            var removeCatalogResult = await this.packageManager.RemovePackageCatalogAsync(removePackageCatalogOptions);
            Assert.IsNotNull(removeCatalogResult);
            Assert.AreEqual(RemovePackageCatalogStatus.Ok, removeCatalogResult.Status);

            var testSource = this.packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            Assert.IsNull(testSource);
        }

        /// <summary>
        /// Add package catalog with invalid options.
        /// </summary>
        [Test]
        public void AddPackageCatalogWithInvalidOptions()
        {
            // Add package catalog with null options.
            Assert.ThrowsAsync<NullReferenceException>(async () => await this.packageManager.AddPackageCatalogAsync(null));

            // Add package catalog with empty options.
            Assert.ThrowsAsync<ArgumentException>(async () => await this.packageManager.AddPackageCatalogAsync(this.TestFactory.CreateAddPackageCatalogOptions()));
        }

        /// <summary>
        /// Add package catalog with a duplicate name and verify it returns SourceNameExists.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task AddPackageCatalog_DuplicateName_ReturnsSourceNameExists()
        {
            AddPackageCatalogOptions options = this.TestFactory.CreateAddPackageCatalogOptions();
            options.SourceUri = Constants.TestSourceUrl;
            options.Name = Constants.TestSourceName;
            options.TrustLevel = PackageCatalogTrustLevel.Trusted;

            await this.AddAndValidatePackageCatalogAsync(options, AddPackageCatalogStatus.Ok);

            // Add the same package catalog again.
            await this.AddAndValidatePackageCatalogAsync(options, AddPackageCatalogStatus.InvalidOptions, Constants.ErrorCode.ERROR_SOURCE_NAME_ALREADY_EXISTS);

            // Remove the tests source if it exists.
            RemovePackageCatalogOptions removePackageCatalogOptions = this.TestFactory.CreateRemovePackageCatalogOptions();
            removePackageCatalogOptions.Name = Constants.TestSourceName;
            await this.RemoveAndValidatePackageCatalogAsync(removePackageCatalogOptions, RemovePackageCatalogStatus.Ok);
        }

        /// <summary>
        /// Add package catalog with a duplicate source uri and verify it returns SourceArg AlreadyExists.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task AddPackageCatalog_DuplicateSourceUri_ReturnSourceArgAlreadyExists()
        {
            AddPackageCatalogOptions options = this.TestFactory.CreateAddPackageCatalogOptions();
            options.SourceUri = Constants.TestSourceUrl;
            options.Name = Constants.TestSourceName;
            options.TrustLevel = PackageCatalogTrustLevel.Trusted;

            await this.AddAndValidatePackageCatalogAsync(options, AddPackageCatalogStatus.Ok);

            // Add the same package catalog again.
            options.Name = "TestSource2";
            await this.AddAndValidatePackageCatalogAsync(options, AddPackageCatalogStatus.InvalidOptions, Constants.ErrorCode.ERROR_SOURCE_ARG_ALREADY_EXISTS);

            // Remove the tests source if it exists.
            RemovePackageCatalogOptions removePackageCatalogOptions = this.TestFactory.CreateRemovePackageCatalogOptions();
            removePackageCatalogOptions.Name = Constants.TestSourceName;
            await this.RemoveAndValidatePackageCatalogAsync(removePackageCatalogOptions, RemovePackageCatalogStatus.Ok);
        }

        /// <summary>
        /// Add package catalog with invalid source uri.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task AddPackageCatalogWithInvalidSourceUri()
        {
            AddPackageCatalogOptions options = this.TestFactory.CreateAddPackageCatalogOptions();
            options.SourceUri = "InvalidUri";
            options.Name = Constants.TestSourceName;
            options.TrustLevel = PackageCatalogTrustLevel.Trusted;

            await this.AddAndValidatePackageCatalogAsync(options, AddPackageCatalogStatus.InternalError);
        }

        /// <summary>
        /// Add package catalog with insecure source uri.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task AddPackageCatalogWithHttpSourceUri()
        {
            AddPackageCatalogOptions options = this.TestFactory.CreateAddPackageCatalogOptions();
            options.SourceUri = "http://microsoft.com";
            options.Name = "Insecure";
            options.TrustLevel = PackageCatalogTrustLevel.Trusted;

            await this.AddAndValidatePackageCatalogAsync(options, AddPackageCatalogStatus.InvalidOptions, Constants.ErrorCode.ERROR_SOURCE_NOT_SECURE);
        }

        /// <summary>
        /// Add package catalog with invalid type.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task AddPackageCatalogWithInvalidType()
        {
            AddPackageCatalogOptions options = this.TestFactory.CreateAddPackageCatalogOptions();
            options.SourceUri = Constants.TestSourceUrl;
            options.Name = Constants.TestSourceName;
            options.Type = "InvalidType";

            await this.AddAndValidatePackageCatalogAsync(options, AddPackageCatalogStatus.InvalidOptions, Constants.ErrorCode.ERROR_INVALID_SOURCE_TYPE);
        }

        /// <summary>
        /// Add, update and remove package catalog.
        /// </summary>
        /// <returns> representing the asynchronous unit test.</returns>
        [Test]
        public async Task AddUpdateRemovePackageCatalog()
        {
            AddPackageCatalogOptions options = this.TestFactory.CreateAddPackageCatalogOptions();
            options.SourceUri = Constants.TestSourceUrl;
            options.Name = Constants.TestSourceName;
            options.TrustLevel = PackageCatalogTrustLevel.Trusted;

            await this.AddCatalogAndVerifyStatusAsync(options, AddPackageCatalogStatus.Ok);

            var packageCatalog = this.GetAndValidatePackageCatalog(options);
            var lastUpdatedTime = packageCatalog.Info.LastUpdateTime;

            // Sleep for 30 seconds to make sure the last updated time is different after the refresh.
            Thread.Sleep(TimeSpan.FromSeconds(30));

            var updateResult = await packageCatalog.RefreshPackageCatalogAsync();
            Assert.IsNotNull(updateResult);
            Assert.AreEqual(RefreshPackageCatalogStatus.Ok, updateResult.Status);

            packageCatalog = this.GetAndValidatePackageCatalog(options);
            Assert.IsTrue(packageCatalog.Info.LastUpdateTime > lastUpdatedTime);

            RemovePackageCatalogOptions removePackageCatalogOptions = this.TestFactory.CreateRemovePackageCatalogOptions();
            removePackageCatalogOptions.Name = Constants.TestSourceName;
            await this.RemoveAndValidatePackageCatalogAsync(removePackageCatalogOptions, RemovePackageCatalogStatus.Ok);
        }

        /// <summary>
        /// Add, remove package catalog with PreserveData filed set..
        /// </summary>
        /// <returns> representing the asynchronous unit test.</returns>
        [Test]
        public async Task AddRemovePackageCatalogWithPreserveDataFiledSet()
        {
            AddPackageCatalogOptions options = this.TestFactory.CreateAddPackageCatalogOptions();
            options.SourceUri = Constants.TestSourceUrl;
            options.Name = Constants.TestSourceName;
            options.TrustLevel = PackageCatalogTrustLevel.Trusted;

            await this.AddAndValidatePackageCatalogAsync(options, AddPackageCatalogStatus.Ok);

            RemovePackageCatalogOptions removePackageCatalogOptions = this.TestFactory.CreateRemovePackageCatalogOptions();
            removePackageCatalogOptions.Name = Constants.TestSourceName;
            removePackageCatalogOptions.PreserveData = true;

            await this.RemoveAndValidatePackageCatalogAsync(removePackageCatalogOptions, RemovePackageCatalogStatus.Ok);
        }

        /// <summary>
        /// Remove package catalog with invalid options.
        /// </summary>
        [Test]
        public void RemovePackageCatalogWithInvalidOptions()
        {
            // Remove package catalog with null options.
            Assert.ThrowsAsync<NullReferenceException>(async () => await this.packageManager.RemovePackageCatalogAsync(null));

            // Remove package catalog with empty options.
            Assert.ThrowsAsync<ArgumentException>(async () => await this.packageManager.RemovePackageCatalogAsync(this.TestFactory.CreateRemovePackageCatalogOptions()));
        }

        /// <summary>
        /// Remove a package catalog that is not present.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task RemoveNonExistingPackageCatalog()
        {
            RemovePackageCatalogOptions removePackageCatalogOptions = this.TestFactory.CreateRemovePackageCatalogOptions();
            removePackageCatalogOptions.Name = Constants.TestSourceName;

            await this.RemoveAndValidatePackageCatalogAsync(removePackageCatalogOptions, RemovePackageCatalogStatus.InvalidOptions, Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST);
        }

        /// <summary>
        /// Test class Tear down.
        /// </summary>
        [OneTimeTearDown]
        public void TestClassTearDown()
        {
            // Restore the tests source if it was removed as the affects subsequent tests.
            TestCommon.SetupTestSource();
        }

        private async Task AddCatalogAndVerifyStatusAsync(AddPackageCatalogOptions addPackageCatalogOptions, AddPackageCatalogStatus expectedStatus, int expectedErrorCode = 0)
        {
            var addCatalogResult = await this.packageManager.AddPackageCatalogAsync(addPackageCatalogOptions);
            Assert.IsNotNull(addCatalogResult);
            Assert.AreEqual(expectedStatus, addCatalogResult.Status);

            if (expectedStatus != AddPackageCatalogStatus.Ok && expectedErrorCode != 0)
            {
                Assert.AreEqual(expectedErrorCode, addCatalogResult.ExtendedErrorCode.HResult);
            }
        }

        private PackageCatalogReference GetAndValidatePackageCatalog(AddPackageCatalogOptions addPackageCatalogOptions)
        {
            var packageCatalog = this.packageManager.GetPackageCatalogByName(addPackageCatalogOptions.Name);

            Assert.IsNotNull(packageCatalog);
            Assert.AreEqual(addPackageCatalogOptions.Name, packageCatalog.Info.Name);
            Assert.AreEqual(addPackageCatalogOptions.SourceUri, packageCatalog.Info.Argument);

            return packageCatalog;
        }

        private async Task AddAndValidatePackageCatalogAsync(AddPackageCatalogOptions addPackageCatalogOptions, AddPackageCatalogStatus expectedStatus, int expectedErrorCode = 0)
        {
            // Add the package catalog and verify the status
            var addCatalogResult = await this.packageManager.AddPackageCatalogAsync(addPackageCatalogOptions);
            Assert.IsNotNull(addCatalogResult);
            Assert.AreEqual(expectedStatus, addCatalogResult.Status);

            if (expectedStatus != AddPackageCatalogStatus.Ok)
            {
                // Only validate the error code if the status is not Ok and the expected error code is not 0
                if (expectedErrorCode != 0)
                {
                    Assert.AreEqual(expectedErrorCode, addCatalogResult.ExtendedErrorCode.HResult);
                }

                return;
            }

            // Validate the added package catalog if the status is Ok
            this.GetAndValidatePackageCatalog(addPackageCatalogOptions);
        }

        private async Task RemoveAndValidatePackageCatalogAsync(RemovePackageCatalogOptions removePackageCatalogOptions, RemovePackageCatalogStatus expectedStatus, int expectedErrorCode = 0)
        {
            var removeCatalogResult = await this.packageManager.RemovePackageCatalogAsync(removePackageCatalogOptions);
            Assert.IsNotNull(removeCatalogResult);
            Assert.AreEqual(expectedStatus, removeCatalogResult.Status);

            if (expectedStatus != RemovePackageCatalogStatus.Ok && expectedErrorCode != 0)
            {
                Assert.AreEqual(expectedErrorCode, removeCatalogResult.ExtendedErrorCode.HResult);
                return;
            }

            var packageCatalog = this.packageManager.GetPackageCatalogByName(removePackageCatalogOptions.Name);
            Assert.IsNull(packageCatalog);
        }
    }
}
