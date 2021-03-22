// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using NUnit.Framework;

    public class ImportCommand : BaseCommand
    {
        [SetUp]
        public void Setup()
        {
            InitializeAllFeatures(false);
            ConfigureFeature("import", true);
            ConfigureFeature("export", true);
            CleanupTestExe();
        }

        [TearDown]
        public void TearDown()
        {
            InitializeAllFeatures(false);
        }

        [Test]
        public void ImportSuccessful()
        {
            var result = TestCommon.RunAICLICommand("import", GetTestImportFile("ImportFile-Good.json"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(VerifyTestExeInstalled());
            UninstallTestExe();
        }

        // Ignore while we don't have schema validation
        [Test]
        public void ImportInvalidFile()
        {
            // Verify failure when trying to import with an invalid file
            var result = TestCommon.RunAICLICommand("import", GetTestImportFile("ImportFile-Bad-Invalid.json"));
            Assert.AreEqual(Constants.ErrorCode.APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, result.ExitCode);
            Assert.True(result.StdOut.Contains("JSON file is not valid"));
        }

        [Test]
        public void ImportUnknownSource()
        {
            // Verify failure when trying to import from an unknown source
            var result = TestCommon.RunAICLICommand("import", GetTestImportFile("ImportFile-Bad-UnknownSource.json"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Source required for import is not installed"));
        }

        [Test]
        public void ImportUnavailablePackage()
        {
            // Verify failure when trying to import an unavailable package
            var result = TestCommon.RunAICLICommand("import", GetTestImportFile("ImportFile-Bad-UnknownPackage.json"));
            Assert.AreEqual(Constants.ErrorCode.APPINSTALLER_CLI_ERROR_NOT_ALL_PACKAGES_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("Package not found for import"));
        }

        [Test]
        public void ImportUnavailableVersion()
        {
            // Verify failure when trying to import an unavailable package
            var result = TestCommon.RunAICLICommand("import", GetTestImportFile("ImportFile-Bad-UnknownPackageVersion.json"));
            Assert.AreEqual(Constants.ErrorCode.APPINSTALLER_CLI_ERROR_NOT_ALL_PACKAGES_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("Package not found for import"));
        }

        [Test]
        public void ImportAlreadyInstalled()
        {
            // Verify success with message when trying to import a package that is already installed
            var installDir = TestCommon.GetRandomTestDir();
            TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller -l {installDir}");
            var result = TestCommon.RunAICLICommand("import", $"{GetTestImportFile("ImportFile-Good.json")}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Package is already installed"));
            Assert.False(VerifyTestExeInstalled());
            UninstallTestExe();
        }

        [Test]
        public void ImportExportedFile()
        {
            // Verify success when importing an exported list of packages.
            // First install the test package to ensure it is exported.
            TestCommon.RunAICLICommand("install", Constants.ExeInstallerPackageId);

            var jsonFile = TestCommon.GetRandomTestFile(".json");
            TestCommon.RunAICLICommand("export", $"{jsonFile} -s TestSource");

            // Uninstall the package to ensure we can install it again
            UninstallTestExe();

            // Import the file
            var result = TestCommon.RunAICLICommand("import", jsonFile);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(VerifyTestExeInstalled());
            UninstallTestExe();
        }

        private string GetTestImportFile(string importFileName)
        {
            return TestCommon.GetTestDataFile(Path.Combine("ImportFiles", importFileName));
        }

        private bool VerifyTestExeInstalled(string installDir = null)
        {
            if (string.IsNullOrEmpty(installDir))
            {
                // Default location used by installer
                installDir = Path.GetTempPath();
            }

            return File.Exists(Path.Combine(installDir, Constants.TestExeInstalledFileName));
        }

        private void UninstallTestExe()
        {
            ConfigureFeature("uninstall", true);
            TestCommon.RunAICLICommand("uninstall", Constants.ExeInstallerPackageId);
        }

        private void CleanupTestExe()
        {
            UninstallTestExe();
            File.Delete(Path.Combine(Path.GetTempPath(), Constants.TestExeInstalledFileName));
            File.Delete(Path.Combine(Path.GetTempPath(), Constants.TestExeUninstallerFileName));
        }
    }
}