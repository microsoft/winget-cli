// -----------------------------------------------------------------------------
// <copyright file="ConnectionValidationInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Threading.Tasks;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;
    using Windows.Security.Cryptography.Certificates;
    using Windows.Storage.Streams;

    /// <summary>
    /// Tests for the in-proc connection validation callback on PackageCatalogReference.
    /// These tests require an in-process COM server because ConnectionValidationHandler
    /// can only be set by in-proc callers.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    public class ConnectionValidationInterop : BaseInterop
    {
        private PackageManager packageManager;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConnectionValidationInterop"/> class.
        /// </summary>
        /// <param name="initializer">initializer.</param>
        public ConnectionValidationInterop(IInstanceInitializer initializer)
            : base(initializer)
        {
        }

        /// <summary>
        /// Set up before each test: create a package manager and ensure the REST source is not present.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            GroupPolicyHelper.BypassCertificatePinningForMicrosoftStore.SetNotConfigured();
            TestCommon.RunAICLICommand("source remove", Constants.RestTestSourceName);
            this.packageManager = this.TestFactory.CreatePackageManager();
        }

        /// <summary>
        /// Tear down after each test: remove the REST source if it was added.
        /// </summary>
        [TearDown]
        public void TearDown()
        {
            GroupPolicyHelper.BypassCertificatePinningForMicrosoftStore.SetNotConfigured();
            TestCommon.RunAICLICommand("source remove", Constants.RestTestSourceName);
        }

        /// <summary>
        /// Verifies that the connection validation callback is invoked with a non-null server certificate,
        /// that returning Ok allows the connection to succeed, and that the received certificate matches
        /// the known test server certificate.
        /// </summary>
        /// <returns>The task.</returns>
        [Test]
        public async Task ConnectionValidationCallback_AllowsConnection_ConnectSucceeds()
        {
            var catalogRef = await this.AddRestCatalogAsync();

            byte[] receivedCertDer = null;
            catalogRef.ConnectionValidationHandler = (args) =>
            {
                receivedCertDer = GetCertificateDerBytes(args.ServerCertificate);
                return PackageCatalogConnectionValidationResult.Ok;
            };

            var connectResult = catalogRef.Connect();

            Assert.That(connectResult, Is.Not.Null);
            Assert.That(connectResult.Status, Is.EqualTo(ConnectResultStatus.Ok), "Connect should succeed when handler returns Ok.");
            Assert.That(connectResult.PackageCatalog, Is.Not.Null, "PackageCatalog should be non-null on success.");
            Assert.That(receivedCertDer, Is.Not.Null, "Handler should have been called with a non-null certificate.");

            // Verify the received certificate matches the test server's known certificate.
            if (!string.IsNullOrEmpty(TestSetup.Parameters.LocalServerCertPath) &&
                File.Exists(TestSetup.Parameters.LocalServerCertPath))
            {
                byte[] expectedCertDer = File.ReadAllBytes(TestSetup.Parameters.LocalServerCertPath);
                Assert.That(receivedCertDer, Is.EqualTo(expectedCertDer), "The certificate received in the callback should match the test server's certificate.");
            }
        }

        /// <summary>
        /// Verifies that the connection validation callback is invoked and that returning CertificateRejected
        /// causes the connection to fail.
        /// </summary>
        /// <returns>The task.</returns>
        [Test]
        public async Task ConnectionValidationCallback_RejectsConnection_ConnectFails()
        {
            var catalogRef = await this.AddRestCatalogAsync();

            bool handlerCalled = false;
            catalogRef.ConnectionValidationHandler = (args) =>
            {
                handlerCalled = true;
                return PackageCatalogConnectionValidationResult.CertificateRejected;
            };

            var connectResult = catalogRef.Connect();

            Assert.That(connectResult, Is.Not.Null);
            Assert.That(connectResult.Status, Is.EqualTo(ConnectResultStatus.CatalogError), "Connect should fail when handler rejects the certificate.");
            Assert.That(handlerCalled, Is.True, "Handler should have been called.");
        }

        /// <summary>
        /// Verifies that setting ConnectionValidationHandler on the Microsoft Store catalog is allowed
        /// when the BypassCertificatePinningForMicrosoftStore policy is enabled.
        /// </summary>
        [Test]
        public void ConnectionValidationHandler_MicrosoftStore_PolicyEnabled_Allowed()
        {
            GroupPolicyHelper.BypassCertificatePinningForMicrosoftStore.Enable();

            var catalogRef = this.packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog.MicrosoftStore);
            Assert.That(catalogRef, Is.Not.Null);

            // Policy enabled means the feature is allowed; setting the handler should not throw.
            Assert.That(
                (Action)(() =>
                {
                    catalogRef.ConnectionValidationHandler = (args) => PackageCatalogConnectionValidationResult.Ok;
                }),
                Throws.Nothing);
        }

        /// <summary>
        /// Verifies that setting ConnectionValidationHandler on the Microsoft Store catalog is blocked
        /// when the BypassCertificatePinningForMicrosoftStore policy is disabled.
        /// </summary>
        [Test]
        public void ConnectionValidationHandler_MicrosoftStore_PolicyDisabled_ThrowsBlockedByPolicy()
        {
            GroupPolicyHelper.BypassCertificatePinningForMicrosoftStore.Disable();

            var catalogRef = this.packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog.MicrosoftStore);
            Assert.That(catalogRef, Is.Not.Null);

            Assert.That(
                (Action)(() =>
            {
                catalogRef.ConnectionValidationHandler = (args) => PackageCatalogConnectionValidationResult.Ok;
            }),
                Throws.TypeOf<COMException>()
                    .With.Property("HResult").EqualTo(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY), "Setting ConnectionValidationHandler on MicrosoftStore with policy disabled should return ERROR_BLOCKED_BY_POLICY.");
        }

        /// <summary>
        /// Verifies that setting ConnectionValidationHandler on a non-Store catalog is allowed
        /// even when the BypassCertificatePinningForMicrosoftStore policy is configured.
        /// </summary>
        /// <returns>The task.</returns>
        [Test]
        public async Task ConnectionValidationHandler_NonStoreCatalog_PolicyEnabled_Allowed()
        {
            GroupPolicyHelper.BypassCertificatePinningForMicrosoftStore.Enable();

            var catalogRef = await this.AddRestCatalogAsync();

            // This should not throw — the policy only applies to the MicrosoftStore catalog.
            Assert.That(
                (Action)(() =>
                {
                    catalogRef.ConnectionValidationHandler = (args) => PackageCatalogConnectionValidationResult.Ok;
                }),
                Throws.Nothing);
        }

        /// <summary>
        /// Verifies that setting ConnectionValidationHandler on a non-Store catalog is allowed
        /// even when the BypassCertificatePinningForMicrosoftStore policy is disabled.
        /// </summary>
        /// <returns>The task.</returns>
        [Test]
        public async Task ConnectionValidationHandler_NonStoreCatalog_PolicyDisabled_Allowed()
        {
            GroupPolicyHelper.BypassCertificatePinningForMicrosoftStore.Disable();

            var catalogRef = await this.AddRestCatalogAsync();

            // The policy only applies to the MicrosoftStore catalog.
            Assert.That(
                (Action)(() =>
                {
                    catalogRef.ConnectionValidationHandler = (args) => PackageCatalogConnectionValidationResult.Ok;
                }),
                Throws.Nothing);
        }

        /// <summary>
        /// Verifies that IsConnectionValidationHandlerEnabled returns true for a non-Store catalog
        /// regardless of policy state.
        /// </summary>
        /// <returns>The task.</returns>
        [Test]
        public async Task IsConnectionValidationHandlerEnabled_NonStoreCatalog_ReturnsTrue()
        {
            GroupPolicyHelper.BypassCertificatePinningForMicrosoftStore.Disable();

            var catalogRef = await this.AddRestCatalogAsync();
            Assert.That(catalogRef.IsConnectionValidationHandlerEnabled, Is.True, "Non-store catalog should always report handler enabled.");
        }

        /// <summary>
        /// Verifies that IsConnectionValidationHandlerEnabled returns false for the MicrosoftStore
        /// catalog when the policy is disabled.
        /// </summary>
        [Test]
        public void IsConnectionValidationHandlerEnabled_MicrosoftStore_PolicyDisabled_ReturnsFalse()
        {
            GroupPolicyHelper.BypassCertificatePinningForMicrosoftStore.Disable();

            var catalogRef = this.packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog.MicrosoftStore);
            Assert.That(catalogRef, Is.Not.Null);
            Assert.That(catalogRef.IsConnectionValidationHandlerEnabled, Is.False, "MicrosoftStore catalog with policy disabled should report handler not enabled.");
        }

        /// <summary>
        /// Verifies that IsConnectionValidationHandlerEnabled returns true for the MicrosoftStore
        /// catalog when the policy is enabled.
        /// </summary>
        [Test]
        public void IsConnectionValidationHandlerEnabled_MicrosoftStore_PolicyEnabled_ReturnsTrue()
        {
            GroupPolicyHelper.BypassCertificatePinningForMicrosoftStore.Enable();

            var catalogRef = this.packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog.MicrosoftStore);
            Assert.That(catalogRef, Is.Not.Null);
            Assert.That(catalogRef.IsConnectionValidationHandlerEnabled, Is.True, "MicrosoftStore catalog with policy enabled should report handler enabled.");
        }

        /// <summary>
        /// Verifies that IsConnectionValidationHandlerEnabled returns true for the MicrosoftStore
        /// catalog when the policy is not configured.
        /// </summary>
        [Test]
        public void IsConnectionValidationHandlerEnabled_MicrosoftStore_PolicyNotConfigured_ReturnsTrue()
        {
            // Policy is left at NotConfigured (set by SetUp).
            var catalogRef = this.packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog.MicrosoftStore);
            Assert.That(catalogRef, Is.Not.Null);
            Assert.That(catalogRef.IsConnectionValidationHandlerEnabled, Is.True, "MicrosoftStore catalog with policy not configured should report handler enabled.");
        }

        private static byte[] GetCertificateDerBytes(Certificate certificate)
        {
            if (certificate is null)
            {
                return null;
            }

            var blob = certificate.GetCertificateBlob();
            var bytes = new byte[blob.Length];
            using var reader = DataReader.FromBuffer(blob);
            reader.ReadBytes(bytes);
            return bytes;
        }

        private async Task<PackageCatalogReference> AddRestCatalogAsync()
        {
            var options = this.TestFactory.CreateAddPackageCatalogOptions();
            options.Name = Constants.RestTestSourceName;
            options.SourceUri = Constants.RestTestSourceUrl;
            options.Type = Constants.RestTestSourceType;
            options.TrustLevel = PackageCatalogTrustLevel.Trusted;

            var addResult = await this.packageManager.AddPackageCatalogAsync(options);
            Assert.That(addResult, Is.Not.Null);
            Assert.That(addResult.Status, Is.EqualTo(AddPackageCatalogStatus.Ok), $"Failed to add REST test source. Error: {addResult.ExtendedErrorCode?.HResult:X}");

            var catalogRef = this.packageManager.GetPackageCatalogByName(Constants.RestTestSourceName);
            Assert.That(catalogRef, Is.Not.Null, "REST test source catalog reference should not be null after adding.");
            return catalogRef;
        }
    }

#pragma warning disable SA1402 // File may only contain a single type

    /// <summary>
    /// Verifies that the ConnectionValidationHandler setter is restricted to in-proc callers.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class ConnectionValidationHandlerOutOfProcessInterop : BaseInterop
    {
        private PackageManager packageManager;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConnectionValidationHandlerOutOfProcessInterop"/> class.
        /// </summary>
        /// <param name="initializer">initializer.</param>
        public ConnectionValidationHandlerOutOfProcessInterop(IInstanceInitializer initializer)
            : base(initializer)
        {
        }

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            this.packageManager = this.TestFactory.CreatePackageManager();
        }

        /// <summary>
        /// Verifies that setting ConnectionValidationHandler from an out-of-process caller throws an access denied exception.
        /// </summary>
        [Test]
        public void ConnectionValidationHandler_OutOfProcess_ThrowsAccessDenied()
        {
            // Use the default winget catalog -- we just need any catalog reference.
            var catalogRef = this.packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog.OpenWindowsCatalog);
            Assert.That(catalogRef, Is.Not.Null);

            // Setting the handler from out-of-proc should be rejected with E_ACCESSDENIED.
            Assert.That(
                (Action)(() =>
                {
                    catalogRef.ConnectionValidationHandler = (args) => PackageCatalogConnectionValidationResult.Ok;
                }),
                Throws.TypeOf<UnauthorizedAccessException>());
        }

        /// <summary>
        /// Verifies that IsConnectionValidationHandlerEnabled returns false for out-of-process callers.
        /// </summary>
        [Test]
        public void IsConnectionValidationHandlerEnabled_OutOfProcess_ReturnsFalse()
        {
            var catalogRef = this.packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog.OpenWindowsCatalog);
            Assert.That(catalogRef, Is.Not.Null);
            Assert.That(catalogRef.IsConnectionValidationHandlerEnabled, Is.False, "Out-of-process callers should always see IsConnectionValidationHandlerEnabled as false.");
        }
    }
}
