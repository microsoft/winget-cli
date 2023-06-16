// -----------------------------------------------------------------------------
// <copyright file="DownloadInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using System.IO;
    using System.Threading.Tasks;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;

    /// <summary>
    /// Download interop.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class DownloadInterop : BaseInterop
    {
        private string installDir;
        private PackageManager packageManager;
        private PackageCatalogReference testSource;

        /// <summary>
        /// Initializes a new instance of the <see cref="DownloadInterop"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public DownloadInterop(IInstanceInitializer initializer)
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
        /// Download the installer to the default directory.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DownloadToDirectory()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestExeInstaller");

            // Configure installation
            var downloadOptions = this.TestFactory.CreateDownloadOptions();
            downloadOptions.AcceptPackageAgreements = true;

            // Download
            var downloadResult = await this.packageManager.DownloadPackageAsync(searchResult.CatalogPackage, downloadOptions);

            // Assert
            var packageVersion = "2.0.0.0";
            Assert.AreEqual(DownloadResultStatus.Ok, downloadResult.Status);
            string downloadDir = Path.Combine(TestCommon.GetDefaultDownloadDirectory(), $"{Constants.ExeInstallerPackageId}.{packageVersion}");
            Assert.True(TestCommon.VerifyInstallerDownload(downloadDir, Constants.AppInstallerTestExeInstallerExe));
        }
    }
}
