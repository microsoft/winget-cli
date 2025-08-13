// -----------------------------------------------------------------------------
// <copyright file="Shutdown.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using System.Threading.Tasks;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;
    using WinGetTestCommon;

    /// <summary>
    /// Shutdown testing.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class Shutdown : BaseInterop
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Shutdown"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public Shutdown(IInstanceInitializer initializer)
            : base(initializer)
        {
        }

        /// <summary>
        /// Checks that shutdown will proceed even though an object is active.
        /// </summary>
        [Test]
        public void NoActiveOperations()
        {
            var packageManager = this.TestFactory.CreatePackageManager();

            var servers = WinGetServerInstance.GetInstances();
            Assert.AreEqual(1, servers.Count);

            var server = servers[0];
            Assert.IsTrue(server.HasWindow);

            // This is the call pattern from Windows
            this.SendMessageAndLog(server, WindowMessage.QueryEndSession);
            this.SendMessageAndLog(server, WindowMessage.EndSession);
            this.SendMessageAndLog(server, WindowMessage.Close);

            Assert.IsTrue(server.Process.WaitForExit(5000));
        }

        /// <summary>
        /// Checks that shutdown will proceed even though an operation is active.
        /// </summary>
        /// <returns>The task.</returns>
        [Test]
        public async Task ActiveInstallOperation()
        {
            var packageManager = this.TestFactory.CreatePackageManager();
            var testSource = packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            var installDir = TestCommon.GetRandomTestDir();

            var servers = WinGetServerInstance.GetInstances();
            Assert.AreEqual(1, servers.Count);

            var server = servers[0];
            Assert.IsTrue(server.HasWindow);

            // Find package
            var searchResult = this.FindOnePackage(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "InapplicableOsVersion");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = installDir;

            // Install
            var installOperation = packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // This is the call pattern from Windows
            this.SendMessageAndLog(server, WindowMessage.QueryEndSession);
            this.SendMessageAndLog(server, WindowMessage.EndSession);
            this.SendMessageAndLog(server, WindowMessage.Close);

            Assert.IsTrue(server.Process.WaitForExit(5000));

            InstallResult installResult = null;
            Exception exception = null;

            try
            {
                installResult = await installOperation;
            }
            catch (Exception ex)
            {
                exception = ex;
            }

            // We just expect some kind of signal to indicate the failed attempt.
            if (installResult != null)
            {
                Assert.AreNotEqual(InstallResultStatus.Ok, installResult.Status);
            }
            else
            {
                Assert.NotNull(exception);
            }

            Assert.False(TestCommon.VerifyTestExeInstalledAndCleanup(installDir));
        }

        private void SendMessageAndLog(WinGetServerInstance server, WindowMessage message)
        {
            TestContext.Out.WriteLine($"Sending message {message} to process {server.Process.Id}...");
            try
            {
                if (server.SendMessage(message))
                {
                    TestContext.Out.WriteLine("... succeeded.");
                }
                else
                {
                    TestContext.Out.WriteLine("... failed.");
                }
            }
            catch (Exception e)
            {
                TestContext.Out.WriteLine($"... had exception: {e.Message}");
            }
        }
    }
}
