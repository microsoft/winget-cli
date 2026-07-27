// -----------------------------------------------------------------------------
// <copyright file="ConfigureExportCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;
    using NUnit.Framework.Internal;

    /// <summary>
    /// `Configure export` command tests.
    /// </summary>
    public class ConfigureExportCommand
    {
        private const string Command = "configure export";
        private const string ShowCommand = "configure show";

        private string previousPathValue = string.Empty;

        /// <summary>
        /// Set up.
        /// </summary>
        [OneTimeSetUp]
        public void BaseSetup()
        {
            TestCommon.SetupTestSource(false);
            var installDir = TestCommon.GetRandomTestDir();
            TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestPackageExport -v 1.0.0.0 --silent -l {installDir}");
            this.previousPathValue = System.Environment.GetEnvironmentVariable("PATH");

            // The installer puts DSCv3 resources in both locations
            System.Environment.SetEnvironmentVariable("PATH", this.previousPathValue + ";" + installDir + ";" + installDir + "\\SubDirectory");
            DSCv3ResourceTestBase.EnsureTestResourcePresence();
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [OneTimeTearDown]
        public void BaseTeardown()
        {
            TestCommon.RunAICLICommand("uninstall", "AppInstallerTest.TestPackageExport");
            TestCommon.TearDownTestSource();
            if (!string.IsNullOrEmpty(this.previousPathValue))
            {
                System.Environment.SetEnvironmentVariable("PATH", this.previousPathValue);
            }
        }

        /// <summary>
        /// Export a specific package.
        /// </summary>
        [Test]
        public void ExportTestPackage()
        {
            var exportDir = TestCommon.GetRandomTestDir();
            var exportFile = Path.Combine(exportDir, "exported.yml");
            var result = TestCommon.RunAICLICommand(Command, $"--package-id AppInstallerTest.TestPackageExport -o {exportFile}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(exportFile, Does.Exist);

            // Check exported file is readable and validate content
            var showResult = TestCommon.RunAICLICommand(ShowCommand, $"-f {exportFile}");
            Assert.That(showResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(showResult.StdOut, Does.Contain("Microsoft.WinGet.Dev/Source"));
            Assert.That(showResult.StdOut, Does.Contain($"[{Constants.TestSourceName}_{Constants.TestSourceType}]"));
            Assert.That(showResult.StdOut, Does.Contain($"type: {Constants.TestSourceType}"));
            Assert.That(showResult.StdOut, Does.Contain($"argument: {Constants.TestSourceUrl}"));
            Assert.That(showResult.StdOut, Does.Contain($"name: {Constants.TestSourceName}"));

            Assert.That(showResult.StdOut, Does.Contain("Microsoft.WinGet.Dev/Package"));
            Assert.That(showResult.StdOut, Does.Contain($"[{Constants.TestSourceName}_AppInstallerTest.TestPackageExport]"));
            Assert.That(showResult.StdOut, Does.Contain($"Dependencies: {Constants.TestSourceName}_{Constants.TestSourceType}"));
            Assert.That(showResult.StdOut, Does.Contain("id: AppInstallerTest.TestPackageExport"));
            Assert.That(showResult.StdOut, Does.Contain($"source: {Constants.TestSourceName}"));
        }

        /// <summary>
        /// Export a specific package with related configuration.
        /// </summary>
        [Test]
        public void ExportTestPackageWithPackageSettings()
        {
            var exportDir = TestCommon.GetRandomTestDir();
            var exportFile = Path.Combine(exportDir, "exported.yml");
            var result = TestCommon.RunAICLICommand(Command, $"--package-id AppInstallerTest.TestPackageExport --module AppInstallerTest --resource TestResource -o {exportFile}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(exportFile, Does.Exist);

            // Check exported file is readable and validate content
            var showResult = TestCommon.RunAICLICommand(ShowCommand, $"-f {exportFile}");
            Assert.That(showResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(showResult.StdOut, Does.Contain("Microsoft.WinGet.Dev/Source"));
            Assert.That(showResult.StdOut, Does.Contain($"[{Constants.TestSourceName}_{Constants.TestSourceType}]"));
            Assert.That(showResult.StdOut, Does.Contain($"type: {Constants.TestSourceType}"));
            Assert.That(showResult.StdOut, Does.Contain($"argument: {Constants.TestSourceUrl}"));
            Assert.That(showResult.StdOut, Does.Contain($"name: {Constants.TestSourceName}"));

            Assert.That(showResult.StdOut, Does.Contain("Microsoft.WinGet.Dev/Package"));
            Assert.That(showResult.StdOut, Does.Contain($"[{Constants.TestSourceName}_AppInstallerTest.TestPackageExport]"));
            Assert.That(showResult.StdOut, Does.Contain($"Dependencies: {Constants.TestSourceName}_{Constants.TestSourceType}"));
            Assert.That(showResult.StdOut, Does.Contain("id: AppInstallerTest.TestPackageExport"));
            Assert.That(showResult.StdOut, Does.Contain($"source: {Constants.TestSourceName}"));

            Assert.That(showResult.StdOut, Does.Contain("AppInstallerTest/TestResource"));
            Assert.That(showResult.StdOut, Does.Contain($"Dependencies: {Constants.TestSourceName}_AppInstallerTest.TestPackageExport"));
            Assert.That(showResult.StdOut, Does.Contain("data: TestData"));
        }

        /// <summary>
        /// Export a specific package with version.
        /// </summary>
        [Test]
        public void ExportTestPackageWithVersion()
        {
            var exportDir = TestCommon.GetRandomTestDir();
            var exportFile = Path.Combine(exportDir, "exported.yml");
            var result = TestCommon.RunAICLICommand(Command, $"--package-id AppInstallerTest.TestPackageExport --include-versions -o {exportFile}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(exportFile, Does.Exist);

            // Check exported file is readable and validate content
            var showResult = TestCommon.RunAICLICommand(ShowCommand, $"-f {exportFile}");
            Assert.That(showResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(showResult.StdOut, Does.Contain("Microsoft.WinGet.Dev/Source"));
            Assert.That(showResult.StdOut, Does.Contain($"[{Constants.TestSourceName}_{Constants.TestSourceType}]"));
            Assert.That(showResult.StdOut, Does.Contain($"type: {Constants.TestSourceType}"));
            Assert.That(showResult.StdOut, Does.Contain($"argument: {Constants.TestSourceUrl}"));
            Assert.That(showResult.StdOut, Does.Contain($"name: {Constants.TestSourceName}"));

            Assert.That(showResult.StdOut, Does.Contain("Microsoft.WinGet.Dev/Package"));
            Assert.That(showResult.StdOut, Does.Contain($"[{Constants.TestSourceName}_AppInstallerTest.TestPackageExport]"));
            Assert.That(showResult.StdOut, Does.Contain($"Dependencies: {Constants.TestSourceName}_{Constants.TestSourceType}"));
            Assert.That(showResult.StdOut, Does.Contain("id: AppInstallerTest.TestPackageExport"));
            Assert.That(showResult.StdOut, Does.Contain($"source: {Constants.TestSourceName}"));
            Assert.That(showResult.StdOut, Does.Contain("version: 1.0.0.0"));
        }

        /// <summary>
        /// Export all.
        /// </summary>
        [Test]
        [Ignore("DSC 3.2 design changes ", Until = "2026-05-10")]
        public void ExportAll()
        {
            var exportDir = TestCommon.GetRandomTestDir();
            var exportFile = Path.Combine(exportDir, "exported.yml");
            var result = TestCommon.RunAICLICommand(Command, $"--all --verbose -o {exportFile}", timeOut: 1200000);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(exportFile, Does.Exist);

            // Check exported file is readable and validate content
            var showResult = TestCommon.RunAICLICommand(ShowCommand, $"-f {exportFile}", timeOut: 1200000);
            Assert.That(showResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            Assert.That(showResult.StdOut, Does.Contain("Microsoft.PowerShell"));

            Assert.That(showResult.StdOut, Does.Contain("Microsoft.WinGet.Dev/UserSettingsFile"));
            Assert.That(showResult.StdOut, Does.Contain("Microsoft.WinGet.Dev/AdminSettings"));
            Assert.That(showResult.StdOut, Does.Contain("Microsoft.Windows.Settings/WindowsSettings"));

            Assert.That(showResult.StdOut, Does.Contain("Microsoft.WinGet.Dev/Source"));
            Assert.That(showResult.StdOut, Does.Contain($"[{Constants.TestSourceName}_{Constants.TestSourceType}]"));
            Assert.That(showResult.StdOut, Does.Contain($"type: {Constants.TestSourceType}"));
            Assert.That(showResult.StdOut, Does.Contain($"argument: {Constants.TestSourceUrl}"));
            Assert.That(showResult.StdOut, Does.Contain($"name: {Constants.TestSourceName}"));

            Assert.That(showResult.StdOut, Does.Contain("Microsoft.WinGet.Dev/Package"));
            Assert.That(showResult.StdOut, Does.Contain($"[{Constants.TestSourceName}_AppInstallerTest.TestPackageExport]"));
            Assert.That(showResult.StdOut, Does.Contain($"Dependencies: {Constants.TestSourceName}_{Constants.TestSourceType}"));
            Assert.That(showResult.StdOut, Does.Contain("id: AppInstallerTest.TestPackageExport"));
            Assert.That(showResult.StdOut, Does.Contain($"source: {Constants.TestSourceName}"));

            Assert.That(showResult.StdOut, Does.Contain("AppInstallerTest/TestResource"));
            Assert.That(showResult.StdOut, Does.Contain($"Dependencies: {Constants.TestSourceName}_AppInstallerTest.TestPackageExport"));
            Assert.That(showResult.StdOut, Does.Contain("data: TestData"));

            Assert.That(showResult.StdOut, Does.Contain("AppInstallerTest/TestResource_SubDirectory"));
            Assert.That(showResult.StdOut, Does.Contain($"Dependencies: {Constants.TestSourceName}_AppInstallerTest.TestPackageExport"));
            Assert.That(showResult.StdOut, Does.Contain("data: TestData"));
        }

        /// <summary>
        /// Export a specific package that's not installed.
        /// </summary>
        [Test]
        public void ExportFailedWithNotFoundPackage()
        {
            var exportDir = TestCommon.GetRandomTestDir();
            var exportFile = Path.Combine(exportDir, "exported.yml");
            var result = TestCommon.RunAICLICommand(Command, $"--package-id NotFound.NotFound -o {exportFile}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));
        }

        /// <summary>
        /// Export all with specific package id.
        /// </summary>
        [Test]
        public void ExportFailedWithAllAndSpecificPackage()
        {
            var exportDir = TestCommon.GetRandomTestDir();
            var exportFile = Path.Combine(exportDir, "exported.yml");
            var result = TestCommon.RunAICLICommand(Command, $"--all --package-id AppInstallerTest.TestPackageExport -o {exportFile}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_INVALID_CL_ARGUMENTS));
        }
    }
}
