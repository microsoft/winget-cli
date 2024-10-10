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
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;
    using Windows.System;

    /// <summary>
    /// Download interop.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class DownloadInterop : BaseInterop
    {
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
        /// Downloads the test installer and its package dependencies.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DownloadDependencies()
        {
            // Find package
            var downloadDir = TestCommon.GetRandomTestDir();
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.PackageDependency");

            // Configure installation
            var downloadOptions = this.TestFactory.CreateDownloadOptions();
            downloadOptions.AcceptPackageAgreements = true;
            downloadOptions.DownloadDirectory = downloadDir;

            // Download
            var downloadResult = await this.packageManager.DownloadPackageAsync(searchResult.CatalogPackage, downloadOptions);

            // Assert
            Assert.AreEqual(DownloadResultStatus.Ok, downloadResult.Status);
            var dependenciesDir = Path.Combine(downloadDir, Constants.Dependencies);
            TestCommon.AssertInstallerDownload(dependenciesDir, "TestPortableExe", "3.0.0.0", ProcessorArchitecture.X64, TestCommon.Scope.Unknown, PackageInstallerType.Portable, "en-US");
            TestCommon.AssertInstallerDownload(downloadDir, "TestPackageDependency", "1.0.0.0", ProcessorArchitecture.X64, TestCommon.Scope.Unknown, PackageInstallerType.Exe, "en-US");
        }

        /// <summary>
        /// Downloads the test installer and skips dependencies.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DownloadDependencies_Skip()
        {
            // Find package
            var downloadDir = TestCommon.GetRandomTestDir();
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.PackageDependency");

            // Configure installation
            var downloadOptions = this.TestFactory.CreateDownloadOptions();
            downloadOptions.AcceptPackageAgreements = true;
            downloadOptions.DownloadDirectory = downloadDir;
            downloadOptions.SkipDependencies = true;

            // Download
            var downloadResult = await this.packageManager.DownloadPackageAsync(searchResult.CatalogPackage, downloadOptions);

            // Assert
            Assert.AreEqual(DownloadResultStatus.Ok, downloadResult.Status);
            var dependenciesDir = Path.Combine(downloadDir, Constants.Dependencies);
            Assert.IsFalse(Directory.Exists(dependenciesDir));
            TestCommon.AssertInstallerDownload(downloadDir, "TestPackageDependency", "1.0.0.0", ProcessorArchitecture.X64, TestCommon.Scope.Unknown, PackageInstallerType.Exe, "en-US");
        }

        /// <summary>
        /// Download the installer to the default directory.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DownloadToDefaultDirectory()
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
            string downloadDir = Path.Combine(TestCommon.GetDefaultDownloadDirectory(), $"{Constants.ExeInstallerPackageId}_{packageVersion}");
            TestCommon.AssertInstallerDownload(downloadDir, "TestExeInstaller", packageVersion, ProcessorArchitecture.X86, TestCommon.Scope.Unknown, PackageInstallerType.Exe);
        }

        /// <summary>
        /// Download the installer to a specified directory.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DownloadToDirectory()
        {
            // Find package
            var downloadDir = TestCommon.GetRandomTestDir();
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestExeInstaller");

            // Configure installation
            var downloadOptions = this.TestFactory.CreateDownloadOptions();
            downloadOptions.AcceptPackageAgreements = true;
            downloadOptions.DownloadDirectory = downloadDir;

            // Download
            var downloadResult = await this.packageManager.DownloadPackageAsync(searchResult.CatalogPackage, downloadOptions);

            // Assert
            Assert.AreEqual(DownloadResultStatus.Ok, downloadResult.Status);
            TestCommon.AssertInstallerDownload(downloadDir, "TestExeInstaller", "2.0.0.0", ProcessorArchitecture.X86, TestCommon.Scope.Unknown, PackageInstallerType.Exe);
        }

        /// <summary>
        /// Download the installer using the user scope argument.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DownloadWithUserScope()
        {
            // Find package
            var downloadDir = TestCommon.GetRandomTestDir();
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestMultipleInstallers");

            // Configure installation
            var downloadOptions = this.TestFactory.CreateDownloadOptions();
            downloadOptions.AcceptPackageAgreements = true;
            downloadOptions.DownloadDirectory = downloadDir;
            downloadOptions.Scope = PackageInstallScope.User;

            // Download
            var downloadResult = await this.packageManager.DownloadPackageAsync(searchResult.CatalogPackage, downloadOptions);

            // Assert
            Assert.AreEqual(DownloadResultStatus.Ok, downloadResult.Status);
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.X64, TestCommon.Scope.User, PackageInstallerType.Nullsoft, "en-US");
        }

        /// <summary>
        /// Download the installer using the machine scope argument.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DownloadWithMachineScope()
        {
            // Find package
            var downloadDir = TestCommon.GetRandomTestDir();
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestMultipleInstallers");

            // Configure installation
            var downloadOptions = this.TestFactory.CreateDownloadOptions();
            downloadOptions.AcceptPackageAgreements = true;
            downloadOptions.DownloadDirectory = downloadDir;
            downloadOptions.Scope = PackageInstallScope.System;

            // Download
            var downloadResult = await this.packageManager.DownloadPackageAsync(searchResult.CatalogPackage, downloadOptions);

            // Assert
            Assert.AreEqual(DownloadResultStatus.Ok, downloadResult.Status);
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.X86, TestCommon.Scope.Machine, PackageInstallerType.Msi, "en-US");
        }

        /// <summary>
        /// Download the test installer using the 'zip' installer type argument.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DownloadWithZipInstallerTypeArg()
        {
            // Find package
            var downloadDir = TestCommon.GetRandomTestDir();
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestMultipleInstallers");

            // Configure installation
            var downloadOptions = this.TestFactory.CreateDownloadOptions();
            downloadOptions.AcceptPackageAgreements = true;
            downloadOptions.DownloadDirectory = downloadDir;
            downloadOptions.InstallerType = PackageInstallerType.Zip;

            // Download
            var downloadResult = await this.packageManager.DownloadPackageAsync(searchResult.CatalogPackage, downloadOptions);

            // Assert
            Assert.AreEqual(DownloadResultStatus.Ok, downloadResult.Status);
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.X64, TestCommon.Scope.Unknown, PackageInstallerType.Exe, "zh-CN", true);
        }

        /// <summary>
        /// Downloads the test installer using the installer type argument.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DownloadWithInstallerTypeArg()
        {
            // Find package
            var downloadDir = TestCommon.GetRandomTestDir();
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestMultipleInstallers");

            // Configure installation
            var downloadOptions = this.TestFactory.CreateDownloadOptions();
            downloadOptions.AcceptPackageAgreements = true;
            downloadOptions.DownloadDirectory = downloadDir;
            downloadOptions.InstallerType = PackageInstallerType.Msi;

            // Download
            var downloadResult = await this.packageManager.DownloadPackageAsync(searchResult.CatalogPackage, downloadOptions);

            // Assert
            Assert.AreEqual(DownloadResultStatus.Ok, downloadResult.Status);
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.X86, TestCommon.Scope.Machine, PackageInstallerType.Msi, "en-US");
        }

        /// <summary>
        /// Downloads the test installer using the architecture argument.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DownloadWithArchitectureArg()
        {
            // Find package
            var downloadDir = TestCommon.GetRandomTestDir();
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestMultipleInstallers");

            // Configure installation
            var downloadOptions = this.TestFactory.CreateDownloadOptions();
            downloadOptions.AcceptPackageAgreements = true;
            downloadOptions.DownloadDirectory = downloadDir;
            downloadOptions.Architecture = ProcessorArchitecture.X86;

            // Download
            var downloadResult = await this.packageManager.DownloadPackageAsync(searchResult.CatalogPackage, downloadOptions);

            // Assert
            Assert.AreEqual(DownloadResultStatus.Ok, downloadResult.Status);
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.X86, TestCommon.Scope.Machine, PackageInstallerType.Msi, "en-US");
        }

        /// <summary>
        /// Downloads the test installer using the locale argument.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DownloadWithLocaleArg()
        {
            // Find package
            var downloadDir = TestCommon.GetRandomTestDir();
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestMultipleInstallers");

            // Configure installation
            var downloadOptions = this.TestFactory.CreateDownloadOptions();
            downloadOptions.AcceptPackageAgreements = true;
            downloadOptions.DownloadDirectory = downloadDir;
            downloadOptions.Locale = "zh-CN";

            // Download
            var downloadResult = await this.packageManager.DownloadPackageAsync(searchResult.CatalogPackage, downloadOptions);

            // Assert
            Assert.AreEqual(DownloadResultStatus.Ok, downloadResult.Status);
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.X64, TestCommon.Scope.Unknown, PackageInstallerType.Exe, "zh-CN", true);
        }

        /// <summary>
        /// Downloads the test installer with a hash mismatch.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task DownloadWithHashMismatch()
        {
            // Find package
            var downloadDir = TestCommon.GetRandomTestDir();
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestExeSha256Mismatch");

            // Configure installation
            var downloadOptions = this.TestFactory.CreateDownloadOptions();
            downloadOptions.AcceptPackageAgreements = true;
            downloadOptions.DownloadDirectory = downloadDir;

            // Download
            var downloadResult = await this.packageManager.DownloadPackageAsync(searchResult.CatalogPackage, downloadOptions);

            // Assert
            Assert.AreEqual(DownloadResultStatus.DownloadError, downloadResult.Status);
        }
    }
}
