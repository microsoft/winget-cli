// -----------------------------------------------------------------------------
// <copyright file="RepairInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;

    /// <summary>
    /// Repair Interop Tests for COM/WinRT Interop classes.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class RepairInterop : BaseInterop
    {
        private string installDir;
        private PackageManager packageManager;
        private PackageCatalogReference compositeSource;

        /// <summary>
        /// Initializes a new instance of the <see cref="RepairInterop"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public RepairInterop(IInstanceInitializer initializer)
            : base(initializer)
        {
        }

        /// <summary>
        /// Test setup.
        /// </summary>
        [SetUp]
        public void Init()
        {
            this.packageManager = this.TestFactory.CreatePackageManager();
            this.installDir = TestCommon.GetRandomTestDir();

            // Create a composite source
            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            var testSource = this.packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            Assert.NotNull(testSource, $"{Constants.TestSourceName} cannot be null");
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            this.compositeSource = this.packageManager.CreateCompositePackageCatalog(options);
        }

        /// <summary>
        /// Test Repair MSI Installer.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test. </returns>
        [Test]
        public async Task RepairMSIInstaller()
        {
            string msiInstallerPackageId = "AppInstallerTest.TestMsiRepair";
            var searchResult = await this.FindAndInstallPackage(msiInstallerPackageId, this.installDir, null);

            // Note: The 'msiexec repair' command requires the original installer file to be present at the location registered in the ARP (Add/Remove Programs).
            // In our test scenario, the MSI installer file is initially placed in a temporary location and then deleted, which can cause the repair operation to fail.
            // To work around this, we copy the installer file to the ARP source directory before running the repair command.
            // A more permanent solution would be to modify the MSI installer to cache the installer file in a known location and register that location as the installer source.
            // This would allow the 'msiexec repair' command to function as expected.
            string installerSourceDir = TestCommon.CopyInstallerFileToARPInstallSourceDirectory(TestCommon.GetTestDataFile("AppInstallerTestMsiInstallerV2.msi"), Constants.MsiInstallerProductCode, true);

            // Repair the package
            await this.RepairPackageAndValidateStatus(searchResult.CatalogPackage, this.TestFactory.CreateRepairOptions(), RepairResultStatus.Ok);

            // Cleanup
            Assert.True(TestCommon.VerifyTestMsiInstalledAndCleanup(this.installDir));

            if (installerSourceDir != null && Directory.Exists(installerSourceDir))
            {
                Directory.Delete(installerSourceDir, true);
            }
        }

        /// <summary>
        /// Test Repair MSIX Installer.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task RepairNonStoreMSIXPackage()
        {
            string msixPackageId = "AppInstallerTest.TestMsixInstaller";
            var searchResult = await this.FindAndInstallPackage(msixPackageId, this.installDir, null);

            // Repair the package
            await this.RepairPackageAndValidateStatus(searchResult.CatalogPackage, this.TestFactory.CreateRepairOptions(), RepairResultStatus.Ok);

            // Cleanup
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }

        /// <summary>
        /// Test MSIX non-store package repair with machine scope.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task RepairNonStoreMsixPackageWithMachineScope()
        {
            // Find package again, but this time it should be detected as installed
            var findPackages = this.FindAllPackages(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "Microsoft.Paint_8wekyb3d8bbwe");

            if (findPackages.Count == 0)
            {
                Assert.Ignore("Test skipped as Microsoft.Paint_8wekyb3d8bbwe cannot be found.");
            }

            var searchResult = findPackages.First();

            if (searchResult == null ||
                (searchResult != null && searchResult.CatalogPackage == null) ||
                (searchResult != null && searchResult.CatalogPackage.InstalledVersion == null))
            {
                Assert.Ignore("Test skipped as Microsoft.Paint_8wekyb3d8bbwe is not installed.");
            }

            // Repair the package
            var repairOptions = this.TestFactory.CreateRepairOptions();
            repairOptions.PackageRepairScope = PackageRepairScope.System;

            await this.RepairPackageAndValidateStatus(searchResult.CatalogPackage, repairOptions, RepairResultStatus.RepairError, Constants.ErrorCode.ERROR_INSTALL_SYSTEM_NOT_SUPPORTED);
        }

        /// <summary>
        /// Test repair of a Burn installer that has a "modify" repair behavior specified in the manifest.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task RepairBurnInstallerWithModifyBehavior()
        {
            string burnInstallerPackageId = "AppInstallerTest.TestModifyRepair";
            var replaceInstallerArguments = this.GetReplacementArgumentsBasedTestFixtureContext($"/InstallDir {this.installDir} /Version 2.0.0.0 /DisplayName TestModifyRepair");

            var searchResult = await this.FindAndInstallPackage(burnInstallerPackageId, this.installDir, replaceInstallerArguments.ToString());

            // Repair the package
            var repairOptions = this.TestFactory.CreateRepairOptions();
            repairOptions.PackageRepairMode = PackageRepairMode.Silent;

            await this.RepairPackageAndValidateStatus(searchResult.CatalogPackage, repairOptions, RepairResultStatus.Ok);

            // Cleanup
            Assert.True(TestCommon.VerifyTestExeRepairCompletedAndCleanup(this.installDir, "Modify Repair operation"));
        }

        /// <summary>
        /// Tests the repair operation of a Burn installer that was installed in user scope but is being repaired in an admin context.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task RepairBurnInstallerInAdminContextWithUserScopeInstall()
        {
            if (this.TestFactory.Context != ClsidContext.InProc)
            {
                Assert.Ignore("Test is only applicable for InProc context.");
            }

            string burnInstallerPackageId = "AppInstallerTest.TestUserScopeInstallRepairInAdminContext";
            string replaceInstallerArguments = $"/InstallDir {this.installDir} /Version 2.0.0.0 /DisplayName TestUserScopeInstallRepairInAdminContext";

            var searchResult = await this.FindAndInstallPackage(burnInstallerPackageId, this.installDir, replaceInstallerArguments);

            // Repair the package
            var repairOptions = this.TestFactory.CreateRepairOptions();
            repairOptions.PackageRepairMode = PackageRepairMode.Silent;

            await this.RepairPackageAndValidateStatus(searchResult.CatalogPackage, repairOptions, RepairResultStatus.RepairError, Constants.ErrorCode.ERROR_ADMIN_CONTEXT_REPAIR_PROHIBITED);

            // Cleanup
            TestCommon.CleanupTestExeAndDirectory(this.installDir);
        }

        /// <summary>
        /// Test repair of a Exe installer that has a "uninstaller" repair behavior specified in the manifest and NoModify ARP flag set.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task RepairBurnInstallerWithModifyBehaviorAndNoModifyFlag()
        {
            string burnInstallerPackageId = "AppInstallerTest.TestModifyRepairWithNoModify";
            var replaceInstallerArguments = this.GetReplacementArgumentsBasedTestFixtureContext($"/InstallDir {this.installDir} /Version 2.0.0.0 /DisplayName TestModifyRepairWithNoModify");
            replaceInstallerArguments.Append(" /NoModify");

            var searchResult = await this.FindAndInstallPackage(burnInstallerPackageId, this.installDir, replaceInstallerArguments.ToString());

            // Repair the package
            var repairOptions = this.TestFactory.CreateRepairOptions();
            repairOptions.PackageRepairMode = PackageRepairMode.Silent;

            await this.RepairPackageAndValidateStatus(searchResult.CatalogPackage, repairOptions, RepairResultStatus.RepairError, Constants.ErrorCode.ERROR_REPAIR_NOT_SUPPORTED);

            // Cleanup
            TestCommon.CleanupTestExeAndDirectory(this.installDir);
        }

        /// <summary>
        /// Tests the scenario where the repair operation is not supported for Portable Installer type.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task RepairOperationNotSupportedForPortableInstaller()
        {
            string installDir = TestCommon.GetPortablePackagesDirectory();
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var searchResult = await this.FindAndInstallPackage(packageId, null, null);

            // Repair the package
            var repairOptions = this.TestFactory.CreateRepairOptions();
            repairOptions.PackageRepairMode = PackageRepairMode.Silent;

            await this.RepairPackageAndValidateStatus(searchResult.CatalogPackage, repairOptions, RepairResultStatus.RepairError, Constants.ErrorCode.ERROR_REPAIR_NOT_SUPPORTED);

            // If no location specified, default behavior is to create a package directory with the name "{packageId}_{sourceId}"
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Test repair of a Exe installer that has a "uninstaller" repair behavior specified in the manifest.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task RepairExeInstallerWithUninstallerBehavior()
        {
            string exeInstallerPackageId = "AppInstallerTest.UninstallerRepair";
            var replaceInstallerArguments = this.GetReplacementArgumentsBasedTestFixtureContext($"/InstallDir {this.installDir} /Version 2.0.0.0 /DisplayName UninstallerRepair");

            var searchResult = await this.FindAndInstallPackage(exeInstallerPackageId, this.installDir, replaceInstallerArguments.ToString());

            // Repair the package
            await this.RepairPackageAndValidateStatus(searchResult.CatalogPackage, this.TestFactory.CreateRepairOptions(), RepairResultStatus.Ok);

            // Cleanup
            Assert.True(TestCommon.VerifyTestExeRepairCompletedAndCleanup(this.installDir, "Uninstaller Repair operation"));
        }

        /// <summary>
        /// Test repair of a Exe installer that has a "uninstaller" repair behavior specified in the manifest and NoRepair ARP flag set.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task RepairExeInstallerWithUninstallerBehaviorAndNoRepairFlag()
        {
            string exeInstallerPackageId = "AppInstallerTest.UninstallerRepairWithNoRepair";
            var replaceInstallerArguments = this.GetReplacementArgumentsBasedTestFixtureContext($"/InstallDir {this.installDir} /Version 2.0.0.0 /DisplayName UninstallerRepairWithNoRepair");
            replaceInstallerArguments.Append(" /NoRepair");

            var searchResult = await this.FindAndInstallPackage(exeInstallerPackageId, this.installDir, replaceInstallerArguments.ToString());

            // Repair the package
            await this.RepairPackageAndValidateStatus(searchResult.CatalogPackage, this.TestFactory.CreateRepairOptions(), RepairResultStatus.RepairError, Constants.ErrorCode.ERROR_REPAIR_NOT_SUPPORTED);

            // Cleanup
            TestCommon.CleanupTestExeAndDirectory(this.installDir);
        }

        /// <summary>
        /// Test repair of a Nullsoft installer that has a "uninstaller" repair behavior specified in the manifest.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task RepairNullsoftInstallerWithUninstallerBehavior()
        {
            string nullsoftTestPackageId = "AppInstallerTest.NullsoftUninstallerRepair";
            var replaceInstallerArguments = this.GetReplacementArgumentsBasedTestFixtureContext($"/InstallDir {this.installDir} /Version 2.0.0.0 /DisplayName NullsoftUninstallerRepair");

            var searchResult = await this.FindAndInstallPackage(nullsoftTestPackageId, this.installDir, replaceInstallerArguments.ToString());

            // Repair the package
            await this.RepairPackageAndValidateStatus(searchResult.CatalogPackage, this.TestFactory.CreateRepairOptions(), RepairResultStatus.Ok);

            // Cleanup
            Assert.True(TestCommon.VerifyTestExeRepairCompletedAndCleanup(this.installDir, "Uninstaller Repair operation"));
        }

        /// <summary>
        /// Test repair of a Inno installer that has a "installer" repair behavior specified in the manifest.
        /// </summary>
        /// <returns>representing the asynchronous unit test.</returns>
        [Test]
        public async Task RepairInnoInstallerWithInstallerRepairBehavior()
        {
            string innoTestPackageId = string.Empty;
            string replaceInstallerArguments = string.Empty;

            if (this.TestFactory.Context == ClsidContext.InProc)
            {
                innoTestPackageId = "AppInstallerTest.TestInstallerRepair";
                replaceInstallerArguments = $"/InstallDir {this.installDir} /Version 2.0.0.0 /DisplayName TestInstallerRepair /UseHKLM";
            }
            else
            {
                innoTestPackageId = "AppInstallerTest.TestUserScopeInstallerRepair";
                replaceInstallerArguments = $"/InstallDir {this.installDir} /Version 2.0.0.0 /DisplayName TestUserScopeInstallerRepair";
            }

            var searchResult = await this.FindAndInstallPackage(innoTestPackageId, this.installDir, replaceInstallerArguments);

            // Repair the package
            await this.RepairPackageAndValidateStatus(searchResult.CatalogPackage, this.TestFactory.CreateRepairOptions(), RepairResultStatus.Ok);

            // Cleanup
            Assert.True(TestCommon.VerifyTestExeRepairCompletedAndCleanup(this.installDir, "Installer Repair operation"));
        }

        private async Task<MatchResult> FindAndInstallPackage(string packageId, string installDir, string replacementInstallerArguments)
        {
            // Find a package
            var searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);

            // Configure installation
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;

            if (!string.IsNullOrEmpty(installDir))
            {
                installOptions.PreferredInstallLocation = installDir;
            }

            if (!string.IsNullOrEmpty(replacementInstallerArguments))
            {
                installOptions.ReplacementInstallerArguments = replacementInstallerArguments;
            }

            // Install the package
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Find package again, but this time it should be detected as installed
            searchResult = this.FindOnePackage(this.compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, packageId);
            Assert.NotNull(searchResult.CatalogPackage.InstalledVersion);

            // Return the search result
            return searchResult;
        }

        private async Task RepairPackageAndValidateStatus(CatalogPackage package, RepairOptions repairOptions, RepairResultStatus expectedStatus, int expectedErrorCode = 0)
        {
            var repairResult = await this.packageManager.RepairPackageAsync(package, repairOptions);
            Assert.AreEqual(expectedStatus, repairResult.Status);

            if (expectedStatus != RepairResultStatus.Ok)
            {
                Assert.AreEqual(expectedErrorCode, repairResult.ExtendedErrorCode.HResult);
            }
        }

        private StringBuilder GetReplacementArgumentsBasedTestFixtureContext(string replacementArguments)
        {
            var replacementArgsBuilder = new StringBuilder(replacementArguments);

            // For In-proc calls runs in Admin context in the test environment, repair functionality has restriction that user scope installed package cannot be repaired in  admin elevate context. for security concerns
            // This is a test coverage workaround that in proc calls will install the package in machine scope where as out of proc calls will install the package in user scope.
            if (this.TestFactory.Context == ClsidContext.InProc)
            {
                replacementArgsBuilder.Append(" /UseHKLM");
            }

            return replacementArgsBuilder;
        }
    }
}
