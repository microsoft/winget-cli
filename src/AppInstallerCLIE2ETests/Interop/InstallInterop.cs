// -----------------------------------------------------------------------------
// <copyright file="InstallInterop.cs" company="Microsoft Corporation">
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

    /// <summary>
    /// Install interop.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class InstallInterop : BaseInterop
    {
        private string installDir;
        private PackageManager packageManager;
        private PackageCatalogReference testSource;

        /// <summary>
        /// Initializes a new instance of the <see cref="InstallInterop"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public InstallInterop(IInstanceInitializer initializer)
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
            this.installDir = TestCommon.GetRandomTestDir();
        }

        /// <summary>
        /// Install exe.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallExe()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestExeInstaller");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;
            installOptions.AcceptPackageAgreements = true;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(this.installDir));
        }

        /// <summary>
        /// Test install with inapplicable os version.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallExeWithInsufficientMinOsVersion()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "InapplicableOsVersion");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.NoApplicableInstallers, installResult.Status);
            Assert.False(TestCommon.VerifyTestExeInstalledAndCleanup(this.installDir));
        }

        /// <summary>
        /// Test install with hash mismatch.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallExeWithHashMismatch()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestExeSha256Mismatch");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.DownloadError, installResult.Status);
            Assert.False(TestCommon.VerifyTestExeInstalledAndCleanup(this.installDir));
        }

        /// <summary>
        /// Test installing inno installer.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallWithInno()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestInnoInstaller");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(this.installDir));
        }

        /// <summary>
        /// Test installing burn installer.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallBurn()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestBurnInstaller");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(this.installDir));
        }

        /// <summary>
        /// Test installing nullsoft installer.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallNullSoft()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestNullsoftInstaller");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(this.installDir));
        }

        /// <summary>
        /// Test installing msi.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallMSI()
        {
            if (string.IsNullOrEmpty(TestIndex.MsiInstaller))
            {
                Assert.Ignore("MSI installer not available");
            }

            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestMsiInstaller");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            Assert.True(TestCommon.VerifyTestMsiInstalledAndCleanup(this.installDir));
        }

        /// <summary>
        /// Test installing an msix.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallMSIX()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestMsixInstaller");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }

        /// <summary>
        /// Test installing msix with machine scope.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallMSIXMachineScope()
        {
            // TODO: Provision and Deprovision api not supported in build server.
            Assert.Ignore();

            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestMsixInstaller");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallScope = PackageInstallScope.System;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup(true));
        }

        /// <summary>
        /// Test installing msix with signature.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallMSIXWithSignature()
        {
            // Task to investigate installation error
            // TODO: https://task.ms/40489822
            Assert.Ignore();

            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestMsixWithSignatureHash");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }

        /// <summary>
        /// Test installing msix with signature machine scope.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallMSIXWithSignatureMachineScope()
        {
            // TODO: Provision and Deprovision api not supported in build server.
            Assert.Ignore();

            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestMsixWithSignatureHash");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallScope = PackageInstallScope.System;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup(true));
        }

        /// <summary>
        /// Test installing msix with signature hash mismatch.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallMSIXWithSignatureHashMismatch()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestMsixSignatureHashMismatch");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.DownloadError, installResult.Status);
            Assert.False(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }

        /// <summary>
        /// Test installing exe.
        /// </summary>
        [Test]
        public void InstallExeWithAlternateSourceFailure()
        {
            // Add mock source
            TestCommon.RunAICLICommand("source add", "failSearch \"{ \"\"SearchHR\"\": \"\"0x80070002\"\" }\" Microsoft.Test.Configurable --header \"{}\"");

            // Get mock source
            var failSearchSource = this.packageManager.GetPackageCatalogByName("failSearch");

            // Find package
            var searchResult = this.FindAllPackages(failSearchSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestExeInstaller");

            // Assert
            Assert.NotNull(failSearchSource);
            Assert.AreEqual(0, searchResult.Count);

            // Remove mock source
            TestCommon.RunAICLICommand("source remove", "failSearch");
        }

        /// <summary>
        /// Test installing portable exe.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallPortableExe()
        {
            string installDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Packages");
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = $"{Constants.ExeInstaller}.exe";
            string fileName = $"{Constants.ExeInstaller}.exe";

            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExePackageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, Constants.PortableExePackageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test installing portable exe with command.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallPortableExeWithCommand()
        {
            string productCode = Constants.PortableExeWithCommandPackageDirName;
            string fileName = Constants.AppInstallerTestExeInstallerExe;
            string commandAlias = Constants.TestCommandExe;

            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExeWithCommandPackageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PreferredInstallLocation = this.installDir;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            TestCommon.VerifyPortablePackage(this.installDir, commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test installing portable package to existing directory.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallPortableToExistingDirectory()
        {
            var existingDir = Path.Combine(this.installDir, "testDirectory");
            Directory.CreateDirectory(existingDir);

            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string fileName = Constants.AppInstallerTestExeInstallerExe;

            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExePackageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PreferredInstallLocation = existingDir;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            TestCommon.VerifyPortablePackage(existingDir, commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test installing portable package where it fails on clean up.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallPortableFailsWithCleanup()
        {
            if (this.TestFactory.Context == ClsidContext.InProc)
            {
                // Task to investigate validation error when running in-process
                // TODO: https://task.ms/40489822
                Assert.Ignore();
            }

            string winGetDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet");
            string installDir = Path.Combine(winGetDir, "Packages");
            string symlinkDirectory = Path.Combine(winGetDir, "Links");
            string packageDirName = Constants.PortableExePackageDirName;
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string fileName = Constants.AppInstallerTestExeInstallerExe;
            string conflictDirectory = Path.Combine(symlinkDirectory, commandAlias);

            // Create a directory with the same name as the symlink in order to cause install to fail.
            Directory.CreateDirectory(conflictDirectory);

            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExePackageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.InstallError, installResult.Status);
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
            Directory.Delete(conflictDirectory, true);
        }

        /// <summary>
        /// Test installing a package with user scope.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallRequireUserScope()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestExeInstaller");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;
            installOptions.PackageInstallScope = PackageInstallScope.User;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.NoApplicableInstallers, installResult.Status);
        }

        /// <summary>
        /// Test installing package with user scope or unknown.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallRequireUserScopeAndUnknown()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestExeInstaller");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;
            installOptions.PackageInstallScope = PackageInstallScope.UserOrUnknown;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
        }

        /// <summary>
        /// Test installing package with agreements and accepting those agreements.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallWithAgreementsAccepted()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.CatalogPackageMetadata");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;
            installOptions.AcceptPackageAgreements = true;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
        }

        /// <summary>
        /// Test installing package with agreements and not accepting those agreements.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallWithAgreementsNotAccepted()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.CatalogPackageMetadata");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;
            installOptions.AcceptPackageAgreements = false;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.PackageAgreementsNotAccepted, installResult.Status);
        }

        /// <summary>
        /// Test installing a package with a package dependency and passing in the 'skip-dependencies' install option.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallWithSkipDependencies()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.PackageDependency");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;
            installOptions.AcceptPackageAgreements = true;
            installOptions.SkipDependencies = true;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert that only the exe installer is installed and not the portable package dependency.
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(this.installDir));

            string installDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Packages");
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = $"{Constants.ExeInstaller}.exe";
            string fileName = $"{Constants.ExeInstaller}.exe";
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, Constants.PortableExePackageDirName), commandAlias, fileName, productCode, false);
        }

        /// <summary>
        /// Test installing a package with a specific installer type install option.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallWithInstallerType()
        {
            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestMultipleInstallers");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;
            installOptions.InstallerType = PackageInstallerType.Msi;
            installOptions.AcceptPackageAgreements = true;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);
            Assert.True(TestCommon.VerifyTestMsiInstalledAndCleanup(this.installDir));
        }

        /// <summary>
        /// Test to verify the GetApplicableInstaller() COM call returns the correct manifest installer metadata.
        /// </summary>
        [Test]
        public void GetApplicableInstaller()
        {
            // Find package
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.PackageInstallerInfo");
            Assert.AreEqual(1, searchResult.Count);

            // Configure installation
            var catalogPackage = searchResult[0].CatalogPackage;
            var packageVersionId = catalogPackage.AvailableVersions[0];
            var packageVersionInfo = catalogPackage.GetPackageVersionInfo(packageVersionId);

            // Use install options with no applicable match.
            var badInstallOptions = this.TestFactory.CreateInstallOptions();
            badInstallOptions.PackageInstallScope = PackageInstallScope.System;

            Assert.IsNull(packageVersionInfo.GetApplicableInstaller(badInstallOptions));

            // Use install options with valid applicable match.
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallScope = PackageInstallScope.User;
            var packageInstallerInfo = packageVersionInfo.GetApplicableInstaller(installOptions);

            // Assert
            Assert.IsNotNull(packageInstallerInfo);
            Assert.AreEqual(ElevationRequirement.ElevationRequired, packageInstallerInfo.ElevationRequirement);
            Assert.AreEqual(Windows.System.ProcessorArchitecture.X64, packageInstallerInfo.Architecture);
            Assert.AreEqual(PackageInstallerType.Zip, packageInstallerInfo.InstallerType);
            Assert.AreEqual(PackageInstallerType.Exe, packageInstallerInfo.NestedInstallerType);
            Assert.AreEqual(PackageInstallerScope.User, packageInstallerInfo.Scope);
            Assert.AreEqual("en-US", packageInstallerInfo.Locale);
        }

        /// <summary>
        /// Install exe and verify that we can find it as installed after.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallExe_VerifyInstalledCatalog()
        {
            var installedCatalogReference = this.packageManager.GetLocalPackageCatalog(LocalPackageCatalog.InstalledPackages);

            // Ensure package is not installed
            var installedResult = this.FindAllPackages(installedCatalogReference, PackageMatchField.ProductCode, PackageFieldMatchOption.Equals, Constants.ExeInstalledDefaultProductCode);
            Assert.IsNotNull(installedResult);
            Assert.AreEqual(0, installedResult.Count);

            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestExeInstaller");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = this.installDir;
            installOptions.AcceptPackageAgreements = true;

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Check installed catalog after
            this.FindOnePackage(installedCatalogReference, PackageMatchField.ProductCode, PackageFieldMatchOption.Equals, Constants.ExeInstalledDefaultProductCode);

            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(this.installDir));
        }

        /// <summary>
        /// Install msix and verify that we can find it as installed after.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task InstallMSIX_VerifyInstalledCatalog()
        {
            var installedCatalogReference = this.packageManager.GetLocalPackageCatalog(LocalPackageCatalog.InstalledPackages);

            // Ensure package is not installed
            var installedResult = this.FindAllPackages(installedCatalogReference, PackageMatchField.PackageFamilyName, PackageFieldMatchOption.Equals, Constants.MsixInstallerPackageFamilyName);
            Assert.IsNotNull(installedResult);
            Assert.AreEqual(0, installedResult.Count);

            // Find package
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestMsixInstaller");

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();

            // Install
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Check installed catalog after
            this.FindOnePackage(installedCatalogReference, PackageMatchField.PackageFamilyName, PackageFieldMatchOption.Equals, Constants.MsixInstallerPackageFamilyName);

            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }
    }
}