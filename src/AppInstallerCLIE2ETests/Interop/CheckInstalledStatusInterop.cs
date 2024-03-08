// -----------------------------------------------------------------------------
// <copyright file="CheckInstalledStatusInterop.cs" company="Microsoft Corporation">
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
    using WinRT;

    /// <summary>
    /// Tests check installed status.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class CheckInstalledStatusInterop : BaseInterop
    {
        private string installDir;
        private string defaultInstallDir = Path.Combine(Path.GetTempPath(), "TestInstalledStatus");
        private PackageManager packageManager;
        private PackageCatalogReference testSource;

        /// <summary>
        /// Initializes a new instance of the <see cref="CheckInstalledStatusInterop"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public CheckInstalledStatusInterop(IInstanceInitializer initializer)
            : base(initializer)
        {
        }

        /// <summary>
        /// Test setup.
        /// </summary>
        [SetUp]
        public void SetUp()
        {
            this.packageManager = this.TestFactory.CreatePackageManager();
            this.testSource = this.packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            this.installDir = TestCommon.GetRandomTestDir();
        }

        /// <summary>
        /// Clean up.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        [TearDown]
        public async Task Cleanup()
        {
            // Find and uninstall the test package if applicable.
            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(this.testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = this.packageManager.CreateCompositePackageCatalog(options);
            var searchResult = this.FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            if (searchResult.CatalogPackage.InstalledVersion != null)
            {
                var uninstallOptions = this.TestFactory.CreateUninstallOptions();
                var uninstallResult = await this.packageManager.UninstallPackageAsync(searchResult.CatalogPackage, uninstallOptions);
                Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            }

            // Remove default install location
            if (Directory.Exists(this.defaultInstallDir))
            {
                Directory.Delete(this.defaultInstallDir, true);
            }
        }

        /// <summary>
        /// Tests arp entries match.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        [Test]
        public async Task CheckInstalledStatusArpVersionMatched()
        {
            // Find and install the test package.
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.ReplacementInstallerArguments = $"/InstallDir {this.installDir} /ProductID CheckInstalledStatusProductId /DisplayName TestCheckInstalledStatus /Version 1.0";
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Add the data file listed in the manifest
            File.WriteAllText(Path.Combine(this.installDir, "data.txt"), "Test");

            // Search from composite source again after installation
            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(this.testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = this.packageManager.CreateCompositePackageCatalog(options);
            searchResult = this.FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");

            // Check installed status
            var checkResult = await searchResult.CatalogPackage.CheckInstalledStatusAsync();
            Assert.AreEqual(CheckInstalledStatusResultStatus.Ok, checkResult.Status);
            Assert.AreEqual(1, checkResult.PackageInstalledStatus.Count);

            // Installer info
            var installerInstalledStatus = checkResult.PackageInstalledStatus[0];
            Assert.AreEqual(Windows.System.ProcessorArchitecture.Neutral, installerInstalledStatus.InstallerInfo.Architecture);
            Assert.AreEqual(PackageInstallerType.Exe, installerInstalledStatus.InstallerInfo.InstallerType);
            Assert.AreEqual(PackageInstallerType.Unknown, installerInstalledStatus.InstallerInfo.NestedInstallerType);
            Assert.AreEqual(PackageInstallerScope.Unknown, installerInstalledStatus.InstallerInfo.Scope);

            // Installer status
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntry, installerInstalledStatus.InstallerInstalledStatus[0].Type);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[0]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocation, installerInstalledStatus.InstallerInstalledStatus[1].Type);
            Assert.AreEqual(this.installDir, installerInstalledStatus.InstallerInstalledStatus[1].Path);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[1]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[2].Type);
            Assert.AreEqual(Path.Combine(this.installDir, "data.txt"), installerInstalledStatus.InstallerInstalledStatus[2].Path);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[2]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[3].Type);
            Assert.AreEqual(Path.Combine(this.installDir, "TestExeInstalled.txt"), installerInstalledStatus.InstallerInstalledStatus[3].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[3]));
            Assert.AreEqual(InstalledStatusType.DefaultInstallLocation, installerInstalledStatus.InstallerInstalledStatus[4].Type);
            Assert.AreEqual(this.defaultInstallDir, Path.GetFullPath(installerInstalledStatus.InstallerInstalledStatus[4].Path));
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_INSTALL_LOCATION_NOT_FOUND, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[4]));
        }

        /// <summary>
        /// Test arp entries no version matched.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        [Test]
        public async Task CheckInstalledStatusArpVersionNotMatched()
        {
            // Find and install the test package.
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.ReplacementInstallerArguments = $"/InstallDir {this.installDir} /ProductID CheckInstalledStatusProductId /DisplayName TestCheckInstalledStatus /Version 2.0";
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Add the data file listed in the manifest
            File.WriteAllText(Path.Combine(this.installDir, "data.txt"), "Test");

            // Search from composite source again after installation
            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(this.testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = this.packageManager.CreateCompositePackageCatalog(options);
            searchResult = this.FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");

            // Check installed status
            var checkResult = await searchResult.CatalogPackage.CheckInstalledStatusAsync(InstalledStatusType.AllAppsAndFeaturesEntryChecks);
            Assert.AreEqual(CheckInstalledStatusResultStatus.Ok, checkResult.Status);
            Assert.AreEqual(1, checkResult.PackageInstalledStatus.Count);

            var installerInstalledStatus = checkResult.PackageInstalledStatus[0];
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntry, installerInstalledStatus.InstallerInstalledStatus[0].Type);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[0]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocation, installerInstalledStatus.InstallerInstalledStatus[1].Type);
            Assert.AreEqual(this.installDir, installerInstalledStatus.InstallerInstalledStatus[1].Path);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[1]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[2].Type);
            Assert.AreEqual(Path.Combine(this.installDir, "data.txt"), installerInstalledStatus.InstallerInstalledStatus[2].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[2]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[3].Type);
            Assert.AreEqual(Path.Combine(this.installDir, "TestExeInstalled.txt"), installerInstalledStatus.InstallerInstalledStatus[3].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[3]));
        }

        /// <summary>
        /// Test arp entries file not found.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        [Test]
        public async Task CheckInstalledStatusArpVersionMatchedFileNotFound()
        {
            // Find and install the test package.
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.ReplacementInstallerArguments = $"/InstallDir {this.installDir} /ProductID CheckInstalledStatusProductId /DisplayName TestCheckInstalledStatus /Version 1.0";
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Search from composite source again after installation
            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(this.testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = this.packageManager.CreateCompositePackageCatalog(options);
            searchResult = this.FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");

            // Check installed status
            var checkResult = await searchResult.CatalogPackage.CheckInstalledStatusAsync(InstalledStatusType.AllAppsAndFeaturesEntryChecks);
            Assert.AreEqual(CheckInstalledStatusResultStatus.Ok, checkResult.Status);
            Assert.AreEqual(1, checkResult.PackageInstalledStatus.Count);

            var installerInstalledStatus = checkResult.PackageInstalledStatus[0];
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntry, installerInstalledStatus.InstallerInstalledStatus[0].Type);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[0]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocation, installerInstalledStatus.InstallerInstalledStatus[1].Type);
            Assert.AreEqual(this.installDir, installerInstalledStatus.InstallerInstalledStatus[1].Path);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[1]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[2].Type);
            Assert.AreEqual(Path.Combine(this.installDir, "data.txt"), installerInstalledStatus.InstallerInstalledStatus[2].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_NOT_FOUND, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[2]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[3].Type);
            Assert.AreEqual(Path.Combine(this.installDir, "TestExeInstalled.txt"), installerInstalledStatus.InstallerInstalledStatus[3].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[3]));
        }

        /// <summary>
        /// Test arp entries hash mismatch.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        [Test]
        public async Task CheckInstalledStatusArpVersionMatchedFileHashMisMatch()
        {
            // Find and install the test package.
            var searchResult = this.FindOnePackage(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            var installOptions = this.TestFactory.CreateInstallOptions();
            installOptions.ReplacementInstallerArguments = $"/InstallDir {this.installDir} /ProductID CheckInstalledStatusProductId /DisplayName TestCheckInstalledStatus /Version 1.0";
            var installResult = await this.packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Add the data file listed in the manifest
            File.WriteAllText(Path.Combine(this.installDir, "data.txt"), "WrongData");

            // Search from composite source again after installation
            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(this.testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = this.packageManager.CreateCompositePackageCatalog(options);
            searchResult = this.FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");

            // Check installed status
            var checkResult = await searchResult.CatalogPackage.CheckInstalledStatusAsync(InstalledStatusType.AllAppsAndFeaturesEntryChecks);
            Assert.AreEqual(CheckInstalledStatusResultStatus.Ok, checkResult.Status);
            Assert.AreEqual(1, checkResult.PackageInstalledStatus.Count);

            var installerInstalledStatus = checkResult.PackageInstalledStatus[0];
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntry, installerInstalledStatus.InstallerInstalledStatus[0].Type);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[0]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocation, installerInstalledStatus.InstallerInstalledStatus[1].Type);
            Assert.AreEqual(this.installDir, installerInstalledStatus.InstallerInstalledStatus[1].Path);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[1]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[2].Type);
            Assert.AreEqual(Path.Combine(this.installDir, "data.txt"), installerInstalledStatus.InstallerInstalledStatus[2].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_HASH_MISMATCH, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[2]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[3].Type);
            Assert.AreEqual(Path.Combine(this.installDir, "TestExeInstalled.txt"), installerInstalledStatus.InstallerInstalledStatus[3].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[3]));
        }

        /// <summary>
        /// Test arp entries default install location not found.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        [Test]
        public async Task CheckInstalledStatusArpNotFoundDefaultInstallLocationFound()
        {
            // Add the data file listed in the manifest to default install location.
            Directory.CreateDirectory(this.defaultInstallDir);
            File.WriteAllText(Path.Combine(this.defaultInstallDir, "data.txt"), "Test");

            // Search from composite source without installation
            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(this.testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = this.packageManager.CreateCompositePackageCatalog(options);
            var searchResult = this.FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");

            // Check installed status
            var checkResult = await searchResult.CatalogPackage.CheckInstalledStatusAsync();
            Assert.AreEqual(CheckInstalledStatusResultStatus.Ok, checkResult.Status);
            Assert.AreEqual(1, checkResult.PackageInstalledStatus.Count);

            var installerInstalledStatus = checkResult.PackageInstalledStatus[0];
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntry, installerInstalledStatus.InstallerInstalledStatus[0].Type);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_ARP_ENTRY_NOT_FOUND, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[0]));
            Assert.AreEqual(InstalledStatusType.DefaultInstallLocation, installerInstalledStatus.InstallerInstalledStatus[1].Type);
            Assert.AreEqual(this.defaultInstallDir, Path.GetFullPath(installerInstalledStatus.InstallerInstalledStatus[1].Path));
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[1]));
            Assert.AreEqual(InstalledStatusType.DefaultInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[2].Type);
            Assert.AreEqual(Path.Combine(this.defaultInstallDir, "data.txt"), Path.GetFullPath(installerInstalledStatus.InstallerInstalledStatus[2].Path));
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[2]));
            Assert.AreEqual(InstalledStatusType.DefaultInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[3].Type);
            Assert.AreEqual(Path.Combine(this.defaultInstallDir, "TestExeInstalled.txt"), Path.GetFullPath(installerInstalledStatus.InstallerInstalledStatus[3].Path));
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_NOT_FOUND, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[3]));
        }

        // CsWinrt maps success error codes(e.g. INSTALLED_STATUS_INSTALL_LOCATION_NOT_APPLICABLE) to null exception.
        // In this case we cannot get the exact hresult by calling winrt projection api.
        // This method is created to directly get hresult from the InstalledStatus object for the tests to compare.
        private static unsafe int GetHResultFromInstalledStatus(InstalledStatus status)
        {
            if (status.Status != null)
            {
                return status.Status.HResult;
            }
            else
            {
                IObjectReference objRef = ((IWinRTObject)status).NativeObject;
                ABI.System.Exception exception = default;
                (*(delegate* unmanaged[Stdcall] <IntPtr, out global::ABI.System.Exception, int>**)objRef.ThisPtr)[8](objRef.ThisPtr, out exception);
                return exception.hr;
            }
        }
    }
}