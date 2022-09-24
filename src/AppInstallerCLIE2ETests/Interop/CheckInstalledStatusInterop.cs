// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;
    using WinRT;
    using System;
    using System.IO;
    using System.Threading.Tasks;


    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class CheckInstalledStatusInterop : BaseInterop
    {
        private string installDir;
        private string defaultInstallDir = Path.Combine(Path.GetTempPath(), "TestInstalledStatus");
        private PackageManager packageManager;
        private PackageCatalogReference testSource;

        public CheckInstalledStatusInterop(IInstanceInitializer initializer) : base(initializer) { }

        [SetUp]
        public void SetUp()
        {
            packageManager = TestFactory.CreatePackageManager();
            testSource = packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            installDir = TestCommon.GetRandomTestDir();
        }

        [TearDown]
        public async Task Cleanup()
        {
            // Find and uninstall the test package if applicable.
            var options = TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = packageManager.CreateCompositePackageCatalog(options);
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            if (searchResult.CatalogPackage.InstalledVersion != null)
            {
                var uninstallOptions = TestFactory.CreateUninstallOptions();
                var uninstallResult = await packageManager.UninstallPackageAsync(searchResult.CatalogPackage, uninstallOptions);
                Assert.AreEqual(UninstallResultStatus.Ok, uninstallResult.Status);
            }

            // Remove default install location
            if (Directory.Exists(defaultInstallDir))
            {
                Directory.Delete(defaultInstallDir, true);
            }
        }

        [Test]
        public async Task CheckInstalledStatusArpVersionMatched()
        {
            // Find and install the test package.
            var searchResult = FindOnePackage(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.ReplacementInstallerArguments = $"/InstallDir {installDir} /ProductID CheckInstalledStatusProductId /DisplayName TestCheckInstalledStatus /Version 1.0";
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Add the data file listed in the manifest
            File.WriteAllText(Path.Combine(installDir, "data.txt"), "Test");

            // Search from composite source again after installation
            var options = TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = packageManager.CreateCompositePackageCatalog(options);
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");

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
            Assert.AreEqual(installDir, installerInstalledStatus.InstallerInstalledStatus[1].Path);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[1]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[2].Type);
            Assert.AreEqual(Path.Combine(installDir, "data.txt"), installerInstalledStatus.InstallerInstalledStatus[2].Path);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[2]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[3].Type);
            Assert.AreEqual(Path.Combine(installDir, "TestExeInstalled.txt"), installerInstalledStatus.InstallerInstalledStatus[3].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[3]));
            Assert.AreEqual(InstalledStatusType.DefaultInstallLocation, installerInstalledStatus.InstallerInstalledStatus[4].Type);
            Assert.AreEqual(defaultInstallDir, Path.GetFullPath(installerInstalledStatus.InstallerInstalledStatus[4].Path));
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_INSTALL_LOCATION_NOT_FOUND, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[4]));
        }

        [Test]
        public async Task CheckInstalledStatusArpVersionNotMatched()
        {
            // Find and install the test package.
            var searchResult = FindOnePackage(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.ReplacementInstallerArguments = $"/InstallDir {installDir} /ProductID CheckInstalledStatusProductId /DisplayName TestCheckInstalledStatus /Version 2.0";
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Add the data file listed in the manifest
            File.WriteAllText(Path.Combine(installDir, "data.txt"), "Test");

            // Search from composite source again after installation
            var options = TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = packageManager.CreateCompositePackageCatalog(options);
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");

            // Check installed status
            var checkResult = await searchResult.CatalogPackage.CheckInstalledStatusAsync(InstalledStatusType.AllAppsAndFeaturesEntryChecks);
            Assert.AreEqual(CheckInstalledStatusResultStatus.Ok, checkResult.Status);
            Assert.AreEqual(1, checkResult.PackageInstalledStatus.Count);

            var installerInstalledStatus = checkResult.PackageInstalledStatus[0];
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntry, installerInstalledStatus.InstallerInstalledStatus[0].Type);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[0]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocation, installerInstalledStatus.InstallerInstalledStatus[1].Type);
            Assert.AreEqual(installDir, installerInstalledStatus.InstallerInstalledStatus[1].Path);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[1]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[2].Type);
            Assert.AreEqual(Path.Combine(installDir, "data.txt"), installerInstalledStatus.InstallerInstalledStatus[2].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[2]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[3].Type);
            Assert.AreEqual(Path.Combine(installDir, "TestExeInstalled.txt"), installerInstalledStatus.InstallerInstalledStatus[3].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[3]));
        }

        [Test]
        public async Task CheckInstalledStatusArpVersionMatchedFileNotFound()
        {
            // Find and install the test package.
            var searchResult = FindOnePackage(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.ReplacementInstallerArguments = $"/InstallDir {installDir} /ProductID CheckInstalledStatusProductId /DisplayName TestCheckInstalledStatus /Version 1.0";
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Search from composite source again after installation
            var options = TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = packageManager.CreateCompositePackageCatalog(options);
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");

            // Check installed status
            var checkResult = await searchResult.CatalogPackage.CheckInstalledStatusAsync(InstalledStatusType.AllAppsAndFeaturesEntryChecks);
            Assert.AreEqual(CheckInstalledStatusResultStatus.Ok, checkResult.Status);
            Assert.AreEqual(1, checkResult.PackageInstalledStatus.Count);

            var installerInstalledStatus = checkResult.PackageInstalledStatus[0];
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntry, installerInstalledStatus.InstallerInstalledStatus[0].Type);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[0]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocation, installerInstalledStatus.InstallerInstalledStatus[1].Type);
            Assert.AreEqual(installDir, installerInstalledStatus.InstallerInstalledStatus[1].Path);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[1]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[2].Type);
            Assert.AreEqual(Path.Combine(installDir, "data.txt"), installerInstalledStatus.InstallerInstalledStatus[2].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_NOT_FOUND, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[2]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[3].Type);
            Assert.AreEqual(Path.Combine(installDir, "TestExeInstalled.txt"), installerInstalledStatus.InstallerInstalledStatus[3].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[3]));
        }

        [Test]
        public async Task CheckInstalledStatusArpVersionMatchedFileHashMisMatch()
        {
            // Find and install the test package.
            var searchResult = FindOnePackage(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.ReplacementInstallerArguments = $"/InstallDir {installDir} /ProductID CheckInstalledStatusProductId /DisplayName TestCheckInstalledStatus /Version 1.0";
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            Assert.AreEqual(InstallResultStatus.Ok, installResult.Status);

            // Add the data file listed in the manifest
            File.WriteAllText(Path.Combine(installDir, "data.txt"), "WrongData");

            // Search from composite source again after installation
            var options = TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = packageManager.CreateCompositePackageCatalog(options);
            searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");

            // Check installed status
            var checkResult = await searchResult.CatalogPackage.CheckInstalledStatusAsync(InstalledStatusType.AllAppsAndFeaturesEntryChecks);
            Assert.AreEqual(CheckInstalledStatusResultStatus.Ok, checkResult.Status);
            Assert.AreEqual(1, checkResult.PackageInstalledStatus.Count);

            var installerInstalledStatus = checkResult.PackageInstalledStatus[0];
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntry, installerInstalledStatus.InstallerInstalledStatus[0].Type);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[0]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocation, installerInstalledStatus.InstallerInstalledStatus[1].Type);
            Assert.AreEqual(installDir, installerInstalledStatus.InstallerInstalledStatus[1].Path);
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[1]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[2].Type);
            Assert.AreEqual(Path.Combine(installDir, "data.txt"), installerInstalledStatus.InstallerInstalledStatus[2].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_HASH_MISMATCH, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[2]));
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntryInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[3].Type);
            Assert.AreEqual(Path.Combine(installDir, "TestExeInstalled.txt"), installerInstalledStatus.InstallerInstalledStatus[3].Path);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[3]));
        }

        [Test]
        public async Task CheckInstalledStatusArpNotFoundDefaultInstallLocationFound()
        {
            // Add the data file listed in the manifest to default install location.
            Directory.CreateDirectory(defaultInstallDir);
            File.WriteAllText(Path.Combine(defaultInstallDir, "data.txt"), "Test");

            // Search from composite source without installation
            var options = TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(testSource);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeSource = packageManager.CreateCompositePackageCatalog(options);
            var searchResult = FindOnePackage(compositeSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestCheckInstalledStatus");

            // Check installed status
            var checkResult = await searchResult.CatalogPackage.CheckInstalledStatusAsync();
            Assert.AreEqual(CheckInstalledStatusResultStatus.Ok, checkResult.Status);
            Assert.AreEqual(1, checkResult.PackageInstalledStatus.Count);

            var installerInstalledStatus = checkResult.PackageInstalledStatus[0];
            Assert.AreEqual(InstalledStatusType.AppsAndFeaturesEntry, installerInstalledStatus.InstallerInstalledStatus[0].Type);
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_ARP_ENTRY_NOT_FOUND, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[0]));
            Assert.AreEqual(InstalledStatusType.DefaultInstallLocation, installerInstalledStatus.InstallerInstalledStatus[1].Type);
            Assert.AreEqual(defaultInstallDir, Path.GetFullPath(installerInstalledStatus.InstallerInstalledStatus[1].Path));
            Assert.AreEqual(Constants.ErrorCode.S_OK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[1]));
            Assert.AreEqual(InstalledStatusType.DefaultInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[2].Type);
            Assert.AreEqual(Path.Combine(defaultInstallDir, "data.txt"), Path.GetFullPath(installerInstalledStatus.InstallerInstalledStatus[2].Path));
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[2]));
            Assert.AreEqual(InstalledStatusType.DefaultInstallLocationFile, installerInstalledStatus.InstallerInstalledStatus[3].Type);
            Assert.AreEqual(Path.Combine(defaultInstallDir, "TestExeInstalled.txt"), Path.GetFullPath(installerInstalledStatus.InstallerInstalledStatus[3].Path));
            Assert.AreEqual(Constants.ErrorCode.INSTALLED_STATUS_FILE_NOT_FOUND, GetHResultFromInstalledStatus(installerInstalledStatus.InstallerInstalledStatus[3]));
        }

        // CsWinrt maps success error codes(e.g. INSTALLED_STATUS_INSTALL_LOCATION_NOT_APPLICABLE) to null exception.
        // In this case we cannot get the exact hresult by calling winrt projection api.
        // This method is created to directly get hresult from the InstalledStatus object for the tests to compare.
        unsafe private static int GetHResultFromInstalledStatus(InstalledStatus status)
        {
            if (status.Status != null)
            {
                return status.Status.HResult;
            }
            else
            {
                IObjectReference objRef = ((IWinRTObject)status).NativeObject;
                ABI.System.Exception exception = default;
                (*(delegate* unmanaged[Stdcall]<IntPtr, out global::ABI.System.Exception, int>**)objRef.ThisPtr)[8](objRef.ThisPtr, out exception);
                return exception.hr;
            }
        }
    }
}