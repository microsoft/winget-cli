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
            Assert.That(removeCatalogResult, Is.Not.Null);
            Assert.That(removeCatalogResult.Status, Is.EqualTo(RemovePackageCatalogStatus.Ok));

            var testSource = this.packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            Assert.That(testSource, Is.Null);
        }

        /// <summary>
        /// Add package catalog with invalid options.
        /// </summary>
        [Test]
        public void AddPackageCatalogWithInvalidOptions()
        {
            // Add package catalog with null options.
            Assert.That((Func<Task>)(async () => await this.packageManager.AddPackageCatalogAsync(null)), Throws.TypeOf<NullReferenceException>());

            // Add package catalog with empty options.
            Assert.That((Func<Task>)(async () => await this.packageManager.AddPackageCatalogAsync(this.TestFactory.CreateAddPackageCatalogOptions())), Throws.TypeOf<ArgumentException>());
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
            Assert.That(updateResult, Is.Not.Null);
            Assert.That(updateResult.Status, Is.EqualTo(RefreshPackageCatalogStatus.Ok));

            packageCatalog = this.GetAndValidatePackageCatalog(options);
            Assert.That(packageCatalog.Info.LastUpdateTime > lastUpdatedTime, Is.True);

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
            Assert.That((Func<Task>)(async () => await this.packageManager.RemovePackageCatalogAsync(null)), Throws.TypeOf<NullReferenceException>());

            // Remove package catalog with empty options.
            Assert.That((Func<Task>)(async () => await this.packageManager.RemovePackageCatalogAsync(this.TestFactory.CreateRemovePackageCatalogOptions())), Throws.TypeOf<ArgumentException>());
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
        /// Edit package catalog with invalid options.
        /// </summary>
        [Test]
        public void EditPackageCatalogWithInvalidOptions()
        {
            // Edit package catalog with null options.
            Assert.That((Action)(() => this.packageManager.EditPackageCatalog(null)), Throws.TypeOf<NullReferenceException>());

            // Edit package catalog with empty options.
            Assert.That((Action)(() => this.packageManager.EditPackageCatalog(this.TestFactory.CreateEditPackageCatalogOptions())), Throws.TypeOf<ArgumentException>());
        }

        /// <summary>
        /// Edit a package catalog that is not present.
        /// </summary>
        [Test]
        public void EditNonExistingPackageCatalog()
        {
            EditPackageCatalogOptions editPackageCatalogOptions = this.TestFactory.CreateEditPackageCatalogOptions();
            editPackageCatalogOptions.Name = Constants.TestSourceName;

            this.EditAndValidatePackageCatalog(editPackageCatalogOptions, EditPackageCatalogStatus.InvalidOptions, Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST);
        }

        /// <summary>
        /// Edit a package catalog that is not present.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task AddEditRemovePackageCatalog()
        {
            // Add
            AddPackageCatalogOptions options = this.TestFactory.CreateAddPackageCatalogOptions();
            options.SourceUri = Constants.TestSourceUrl;
            options.Name = Constants.TestSourceName;
            options.TrustLevel = PackageCatalogTrustLevel.Trusted;
            options.Explicit = true;
            options.Priority = 12;

            await this.AddAndValidatePackageCatalogAsync(options, AddPackageCatalogStatus.Ok);

            // Edit
            EditPackageCatalogOptions editOptions = this.TestFactory.CreateEditPackageCatalogOptions();
            editOptions.Name = Constants.TestSourceName;
            editOptions.Explicit = false;
            editOptions.Priority = 42;
            this.EditAndValidatePackageCatalog(editOptions, EditPackageCatalogStatus.Ok);

            // Remove
            RemovePackageCatalogOptions removePackageCatalogOptions = this.TestFactory.CreateRemovePackageCatalogOptions();
            removePackageCatalogOptions.Name = Constants.TestSourceName;
            var removeCatalogResult = await this.packageManager.RemovePackageCatalogAsync(removePackageCatalogOptions);
            Assert.That(removeCatalogResult, Is.Not.Null);
            Assert.That(removeCatalogResult.Status, Is.EqualTo(RemovePackageCatalogStatus.Ok));

            var testSource = this.packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            Assert.That(testSource, Is.Null);
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
            Assert.That(addCatalogResult, Is.Not.Null);
            Assert.That(addCatalogResult.Status, Is.EqualTo(expectedStatus));

            if (expectedStatus != AddPackageCatalogStatus.Ok && expectedErrorCode != 0)
            {
                Assert.That(addCatalogResult.ExtendedErrorCode.HResult, Is.EqualTo(expectedErrorCode));
            }
        }

        private PackageCatalogReference GetAndValidatePackageCatalog(AddPackageCatalogOptions addPackageCatalogOptions)
        {
            var packageCatalog = this.packageManager.GetPackageCatalogByName(addPackageCatalogOptions.Name);

            Assert.That(packageCatalog, Is.Not.Null);
            Assert.That(packageCatalog.Info.Name, Is.EqualTo(addPackageCatalogOptions.Name));
            Assert.That(packageCatalog.Info.Argument, Is.EqualTo(addPackageCatalogOptions.SourceUri));
            Assert.That(packageCatalog.Info.Explicit, Is.EqualTo(addPackageCatalogOptions.Explicit));
            Assert.That(packageCatalog.Info.Priority, Is.EqualTo(addPackageCatalogOptions.Priority));

            return packageCatalog;
        }

        private async Task AddAndValidatePackageCatalogAsync(AddPackageCatalogOptions addPackageCatalogOptions, AddPackageCatalogStatus expectedStatus, int expectedErrorCode = 0)
        {
            // Add the package catalog and verify the status
            var addCatalogResult = await this.packageManager.AddPackageCatalogAsync(addPackageCatalogOptions);
            Assert.That(addCatalogResult, Is.Not.Null);
            Assert.That(addCatalogResult.Status, Is.EqualTo(expectedStatus));

            if (expectedStatus != AddPackageCatalogStatus.Ok)
            {
                // Only validate the error code if the status is not Ok and the expected error code is not 0
                if (expectedErrorCode != 0)
                {
                    Assert.That(addCatalogResult.ExtendedErrorCode.HResult, Is.EqualTo(expectedErrorCode));
                }

                return;
            }

            // Validate the added package catalog if the status is Ok
            this.GetAndValidatePackageCatalog(addPackageCatalogOptions);
        }

        private async Task RemoveAndValidatePackageCatalogAsync(RemovePackageCatalogOptions removePackageCatalogOptions, RemovePackageCatalogStatus expectedStatus, int expectedErrorCode = 0)
        {
            var removeCatalogResult = await this.packageManager.RemovePackageCatalogAsync(removePackageCatalogOptions);
            Assert.That(removeCatalogResult, Is.Not.Null);
            Assert.That(removeCatalogResult.Status, Is.EqualTo(expectedStatus));

            if (expectedStatus != RemovePackageCatalogStatus.Ok && expectedErrorCode != 0)
            {
                Assert.That(removeCatalogResult.ExtendedErrorCode.HResult, Is.EqualTo(expectedErrorCode));
                return;
            }

            var packageCatalog = this.packageManager.GetPackageCatalogByName(removePackageCatalogOptions.Name);
            Assert.That(packageCatalog, Is.Null);
        }

        private void EditAndValidatePackageCatalog(EditPackageCatalogOptions editPackageCatalogOptions, EditPackageCatalogStatus expectedStatus, int expectedErrorCode = 0)
        {
            var editCatalogResult = this.packageManager.EditPackageCatalog(editPackageCatalogOptions);
            Assert.That(editCatalogResult, Is.Not.Null);
            Assert.That(editCatalogResult.Status, Is.EqualTo(expectedStatus));

            if (expectedStatus != EditPackageCatalogStatus.Ok && expectedErrorCode != 0)
            {
                Assert.That(editCatalogResult.ExtendedErrorCode.HResult, Is.EqualTo(expectedErrorCode));
                return;
            }

            // Verify edits are correct.
            var packageCatalog = this.packageManager.GetPackageCatalogByName(editPackageCatalogOptions.Name);
            if (editPackageCatalogOptions.Explicit != null)
            {
                Assert.That(editPackageCatalogOptions.Explicit, Is.EqualTo(packageCatalog.Info.Explicit));
            }

            if (editPackageCatalogOptions.Priority != null)
            {
                Assert.That(editPackageCatalogOptions.Priority, Is.EqualTo(packageCatalog.Info.Priority));
            }
        }
    }
}
