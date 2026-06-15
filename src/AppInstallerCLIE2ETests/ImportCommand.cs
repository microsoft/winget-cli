// -----------------------------------------------------------------------------
// <copyright file="ImportCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Import command tests.
    /// </summary>
    public class ImportCommand : BaseCommand
    {
        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            this.CleanupTestExe();
        }

        /// <summary>
        /// Test import v1.
        /// </summary>
        [Test]
        public void ImportSuccessful_1_0()
        {
            var result = TestCommon.RunAICLICommand("import", this.GetTestImportFile("ImportFile-Good.1.0.json"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(this.VerifyTestExeInstalled());
            this.UninstallTestExe();
        }

        /// <summary>
        /// Test import v2.
        /// </summary>
        [Test]
        public void ImportSuccessful_2_0()
        {
            var result = TestCommon.RunAICLICommand("import", this.GetTestImportFile("ImportFile-Good.2.0.json"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(this.VerifyTestExeInstalled());
            this.UninstallTestExe();
        }

        /// <summary>
        /// Test import invalid file.
        /// </summary>
        [Test]
        public void ImportInvalidFile()
        {
            // Verify failure when trying to import with an invalid file
            var result = TestCommon.RunAICLICommand("import", this.GetTestImportFile("ImportFile-Bad-Invalid.json"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_JSON_INVALID_FILE, result.ExitCode);
            Assert.True(result.StdOut.Contains("JSON file is not valid"));
        }

        /// <summary>
        /// Test import from an unknown source.
        /// </summary>
        [Test]
        public void ImportUnknownSource()
        {
            // Verify failure when trying to import from an unknown source
            var result = TestCommon.RunAICLICommand("import", this.GetTestImportFile("ImportFile-Bad-UnknownSource.json"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Source required for import is not installed"));
        }

        /// <summary>
        /// Test import for an unknown package.
        /// </summary>
        [Test]
        public void ImportUnavailablePackage()
        {
            // Verify failure when trying to import an unavailable package
            var result = TestCommon.RunAICLICommand("import", this.GetTestImportFile("ImportFile-Bad-UnknownPackage.json"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_NOT_ALL_QUERIES_FOUND_SINGLE, result.ExitCode);
            Assert.True(result.StdOut.Contains("Package not found: MissingPackage"));
        }

        /// <summary>
        /// Test import when the package version is not present.
        /// </summary>
        [Test]
        public void ImportUnavailableVersion()
        {
            // Verify failure when trying to import an unavailable package
            var result = TestCommon.RunAICLICommand("import", this.GetTestImportFile("ImportFile-Bad-UnknownPackageVersion.json"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_NOT_ALL_QUERIES_FOUND_SINGLE, result.ExitCode);
            Assert.True(result.StdOut.Contains("Search failed for: AppInstallerTest.TestExeInstaller"));
        }

        /// <summary>
        /// Test import when the package is already installed.
        /// </summary>
        [Test]
        public void ImportAlreadyInstalled()
        {
            // Verify success with message when trying to import a package that is already installed
            var installDir = TestCommon.GetRandomTestDir();
            TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller -l {installDir}");
            var result = TestCommon.RunAICLICommand("import", $"{this.GetTestImportFile("ImportFile-Good.1.0.json")}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Package is already installed"));
            Assert.False(this.VerifyTestExeInstalled());
            this.UninstallTestExe();
        }

        /// <summary>
        /// Test Import with an exported file.
        /// </summary>
        [Test]
        public void ImportExportedFile()
        {
            // Verify success when importing an exported list of packages.
            // First install the test package to ensure it is exported.
            TestCommon.RunAICLICommand("install", Constants.ExeInstallerPackageId);

            var jsonFile = TestCommon.GetRandomTestFile(".json");
            TestCommon.RunAICLICommand("export", $"{jsonFile} -s TestSource");

            // Uninstall the package to ensure we can install it again
            this.UninstallTestExe();

            // Import the file
            var result = TestCommon.RunAICLICommand("import", jsonFile);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(this.VerifyTestExeInstalled());
            this.UninstallTestExe();
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
            TestCommon.RunAICLICommand("uninstall", Constants.ExeInstallerPackageId);
        }

        private void CleanupTestExe()
        {
            this.UninstallTestExe();
            File.Delete(Path.Combine(Path.GetTempPath(), Constants.TestExeInstalledFileName));
            File.Delete(Path.Combine(Path.GetTempPath(), Constants.TestExeUninstallerFileName));
        }
    }
}