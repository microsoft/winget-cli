// -----------------------------------------------------------------------------
// <copyright file="ConnectionValidationInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using System.IO;
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

            Assert.IsNotNull(connectResult);
            Assert.AreEqual(ConnectResultStatus.Ok, connectResult.Status, "Connect should succeed when handler returns Ok.");
            Assert.IsNotNull(connectResult.PackageCatalog, "PackageCatalog should be non-null on success.");
            Assert.IsNotNull(receivedCertDer, "Handler should have been called with a non-null certificate.");

            // Verify the received certificate matches the test server's known certificate.
            if (!string.IsNullOrEmpty(TestSetup.Parameters.LocalServerCertPath) &&
                File.Exists(TestSetup.Parameters.LocalServerCertPath))
            {
                byte[] expectedCertDer = File.ReadAllBytes(TestSetup.Parameters.LocalServerCertPath);
                Assert.AreEqual(
                    expectedCertDer,
                    receivedCertDer,
                    "The certificate received in the callback should match the test server's certificate.");
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

            Assert.IsNotNull(connectResult);
            Assert.AreEqual(ConnectResultStatus.CatalogError, connectResult.Status, "Connect should fail when handler rejects the certificate.");
            Assert.IsTrue(handlerCalled, "Handler should have been called.");
        }

        /// <summary>
        /// Verifies that setting ConnectionValidationHandler on the Microsoft Store catalog is blocked
        /// when the BypassCertificatePinningForMicrosoftStore policy is enabled.
        /// </summary>
        [Test]
        public void ConnectionValidationHandler_MicrosoftStore_PolicyEnabled_ThrowsBlockedByPolicy()
        {
            GroupPolicyHelper.BypassCertificatePinningForMicrosoftStore.Enable();

            var catalogRef = this.packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog.MicrosoftStore);
            Assert.IsNotNull(catalogRef);

            var ex = Assert.Throws<Exception>(() =>
            {
                catalogRef.ConnectionValidationHandler = (args) => PackageCatalogConnectionValidationResult.Ok;
            });

            Assert.AreEqual(
                Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY,
                ex.HResult,
                "Setting ConnectionValidationHandler on MicrosoftStore with policy enabled should return ERROR_BLOCKED_BY_POLICY.");
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
            Assert.IsNotNull(catalogRef);

            var ex = Assert.Throws<Exception>(() =>
            {
                catalogRef.ConnectionValidationHandler = (args) => PackageCatalogConnectionValidationResult.Ok;
            });

            Assert.AreEqual(
                Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY,
                ex.HResult,
                "Setting ConnectionValidationHandler on MicrosoftStore with policy disabled should return ERROR_BLOCKED_BY_POLICY.");
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
            Assert.DoesNotThrow(() =>
            {
                catalogRef.ConnectionValidationHandler = (args) => PackageCatalogConnectionValidationResult.Ok;
            });
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
            Assert.IsNotNull(addResult);
            Assert.AreEqual(AddPackageCatalogStatus.Ok, addResult.Status, $"Failed to add REST test source. Error: {addResult.ExtendedErrorCode?.HResult:X}");

            var catalogRef = this.packageManager.GetPackageCatalogByName(Constants.RestTestSourceName);
            Assert.IsNotNull(catalogRef, "REST test source catalog reference should not be null after adding.");
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
            Assert.IsNotNull(catalogRef);

            // Setting the handler from out-of-proc should be rejected with E_ACCESSDENIED.
            var ex = Assert.Throws<Exception>(() =>
            {
                catalogRef.ConnectionValidationHandler = (args) => PackageCatalogConnectionValidationResult.Ok;
            });

            Assert.IsNotNull(ex);

            // The exception should carry E_ACCESSDENIED (0x80070005).
            const int E_ACCESSDENIED = unchecked((int)0x80070005);
            Assert.AreEqual(E_ACCESSDENIED, ex.HResult, "Setting ConnectionValidationHandler out-of-proc should return E_ACCESSDENIED.");
        }
    }
}
