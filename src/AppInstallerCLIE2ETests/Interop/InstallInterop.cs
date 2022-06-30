// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment;
    using NUnit.Framework;
    using System;
    using System.IO;
    using System.Threading.Tasks;
    using WinGetProjection;

    [TestFixtureSource(typeof(InitializersSource), nameof(InitializersSource.InProcess), Category = nameof(InitializersSource.InProcess))]
    [TestFixtureSource(typeof(InitializersSource), nameof(InitializersSource.OutOfProcess), Category = nameof(InitializersSource.OutOfProcess))]
    public class InstallInterop : BaseInterop
    {
        private string installDir;
        private PackageManager packageManager;
        private PackageCatalogReference testSource;

        public InstallInterop(IInstanceInitializer initializer) : base(initializer) { }

        [SetUp]
        public void SetUp()
        {
            packageManager = TestFactory.CreatePackageManager();
            testSource = packageManager.GetPackageCatalogByName(Constants.TestSourceName);
            installDir = TestCommon.GetRandomTestDir();
        }

        public void InstallAppDoesNotExist()
        {
            // Find package
            var searchResult = FindAllPackages(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "DoesNotExist");
            
            // Assert
            Assert.True(0 == searchResult.Count);
        }

        [Test]
        public void InstallWithMultipleAppsMatchingQuery()
        {
            // Find package
            var searchResult = FindAllPackages(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestExeInstaller");
            
            // Assert
            Assert.True(2 == searchResult.Count);
        }

        [Test]
        public async Task InstallExe()
        {
            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestExeInstaller");

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = installDir;
            
            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            
            // Assert
            Assert.True(InstallResultStatus.Ok == installResult.Status);
        }

        [Test]
        public async Task InstallExeWithInsufficientMinOsVersion()
        {
            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "InapplicableOsVersion");

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = installDir;
            
            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.True(InstallResultStatus.NoApplicableInstallers == installResult.Status);
            Assert.False(TestCommon.VerifyTestExeInstalled(installDir));
        }

        [Test]
        public async Task InstallExeWithHashMismatch()
        {
            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestExeSha256Mismatch");

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = installDir;

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.True(InstallResultStatus.DownloadError == installResult.Status);
            Assert.False(TestCommon.VerifyTestExeInstalled(installDir));
        }

        [Test]
        public async Task InstallWithInno()
        {
            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestInnoInstaller");

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = installDir;

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            Assert.True(TestCommon.VerifyTestExeInstalled(installDir));
        }

        [Test]
        public async Task InstallBurn()
        {
            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestBurnInstaller");
            
            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = installDir;

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            
            // Assert
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            Assert.True(TestCommon.VerifyTestExeInstalled(installDir));
        }

        [Test]
        public async Task InstallNullSoft()
        {
            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestNullsoftInstaller");
            
            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = installDir;
            
            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            Assert.True(TestCommon.VerifyTestExeInstalled(installDir));
        }

        [Test]
        public async Task InstallMSI()
        {
            if (string.IsNullOrEmpty(TestCommon.MsiInstallerPath))
            {
                Assert.Ignore("MSI installer not available");
            }

            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestMsiInstaller");
            
            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = installDir;

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            
            // Assert
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            Assert.True(TestCommon.VerifyTestMsiInstalledAndCleanup(installDir));
        }

        [Test]
        public async Task InstallMSIX()
        {
            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestMsixInstaller");
            
            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            
            // Assert
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            Assert.True(TestCommon.VerifyTestMsixInstalledAndCleanup());
        }

        [Test]
        public async Task InstallMSIXWithSignature()
        {
            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestMsixWithSignatureHash");
            
            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PackageInstallMode = PackageInstallMode.Silent;
            installOptions.PreferredInstallLocation = installDir;

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            
            // Assert
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            Assert.True(TestCommon.VerifyTestMsiInstalledAndCleanup(installDir));
        }

        [Test]
        public async Task InstallMSIXWithSignatureHashMismatch()
        {
            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestMsixSignatureHashMismatch");
            
            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();

            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
            
            // Assert
            Assert.True(InstallResultStatus.InstallError == installResult.Status);
            Assert.True(TestCommon.VerifyTestMsiInstalledAndCleanup(installDir));
        }

        [Test]
        public void InstallExeWithAlternateSourceFailure()
        {
            // Add mock source
            TestCommon.RunAICLICommand("source add", "failSearch \"{ \"\"SearchHR\"\": \"\"0x80070002\"\" }\" Microsoft.Test.Configurable --header \"{}\"");

            // Get mock source
            var failSearchSource = packageManager.GetPackageCatalogByName("failSearch");

            // Find package
            var searchResult = FindAllPackages(failSearchSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.TestExeInstaller");

            // Assert
            Assert.NotNull(failSearchSource);
            Assert.True(0 == searchResult.Count);

            // Remove mock source
            TestCommon.RunAICLICommand("source remove", "failSearch");
        }

        [Test]
        public async Task InstallPortableExe()
        {
            string installDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet", "Packages");
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = $"{Constants.ExeInstaller}.exe";
            string fileName = $"{Constants.ExeInstaller}.exe";

            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExePackageId);
            
            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
        
            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, Constants.PortableExePackageDirName), commandAlias, fileName, productCode, true);
        }

        [Test]
        public async Task InstallPortableExeWithCommand()
        {
            string productCode = Constants.PortableExeWithCommandPackageDirName;
            string fileName = Constants.AppInstallerTestExeInstallerExe;
            string commandAlias = Constants.TestCommandExe;

            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExeWithCommandPackageId);

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PreferredInstallLocation = installDir;
        
            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            TestCommon.VerifyPortablePackage(installDir, commandAlias, fileName, productCode, true);
        }

        [Test]
        public async Task InstallPortableToExistingDirectory()
        {
            var existingDir = Path.Combine(installDir, "testDirectory");
            Directory.CreateDirectory(existingDir);

            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe; 
            string fileName = Constants.AppInstallerTestExeInstallerExe;

            // Find package
            var searchResult = FindOnePackage(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExePackageId);

            // Configure installation
            var installOptions = TestFactory.CreateInstallOptions();
            installOptions.PreferredInstallLocation = existingDir;
        
            // Install
            var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);

            // Assert
            Assert.True(InstallResultStatus.Ok == installResult.Status);
            TestCommon.VerifyPortablePackage(existingDir, commandAlias, fileName, productCode, true);
        }

        [Test]
        public async Task InstallPortableFailsWithCleanup()
        {
            // FIXME Failing
            Assert.Ignore();

            string winGetDir = Path.Combine(Environment.GetEnvironmentVariable(Constants.LocalAppData), "Microsoft", "WinGet");
            string installDir = Path.Combine(winGetDir, "Packages");
            string symlinkDirectory = Path.Combine(winGetDir, "Links");
            string packageDirName = Constants.PortableExePackageDirName;
            string productCode = Constants.PortableExePackageDirName;
            string commandAlias = Constants.AppInstallerTestExeInstallerExe;
            string fileName = Constants.AppInstallerTestExeInstallerExe;
            string conflictDirectory = Path.Combine(symlinkDirectory, commandAlias);

            try
            {
                // Create a directory with the same name as the symlink in order to cause install to fail.
                Directory.CreateDirectory(conflictDirectory);

                // Find package
                var searchResult = FindOnePackage(testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, Constants.PortableExePackageId);
                
                // Configure installation
                var installOptions = TestFactory.CreateInstallOptions();
            
                // Install
                var installResult = await packageManager.InstallPackageAsync(searchResult.CatalogPackage, installOptions);
                
                // Assert
                Assert.AreEqual(InstallResultStatus.InstallError, installResult.Status);
                TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, false);
            }
            finally
            {
                Directory.Delete(conflictDirectory, true);
            }
        }
    }
}