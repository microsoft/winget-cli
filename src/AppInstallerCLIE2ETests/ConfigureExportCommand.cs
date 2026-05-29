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
            Assert.That(File.Exists(exportFile), Is.True);

            // Check exported file is readable and validate content
            var showResult = TestCommon.RunAICLICommand(ShowCommand, $"-f {exportFile}");
            Assert.That(showResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(showResult.StdOut.Contains("Microsoft.WinGet.Dev/Source"), Is.True);
            Assert.That(showResult.StdOut.Contains($"[{Constants.TestSourceName}_{Constants.TestSourceType}]"), Is.True);
            Assert.That(showResult.StdOut.Contains($"type: {Constants.TestSourceType}"), Is.True);
            Assert.That(showResult.StdOut.Contains($"argument: {Constants.TestSourceUrl}"), Is.True);
            Assert.That(showResult.StdOut.Contains($"name: {Constants.TestSourceName}"), Is.True);

            Assert.That(showResult.StdOut.Contains("Microsoft.WinGet.Dev/Package"), Is.True);
            Assert.That(showResult.StdOut.Contains($"[{Constants.TestSourceName}_AppInstallerTest.TestPackageExport]"), Is.True);
            Assert.That(showResult.StdOut.Contains($"Dependencies: {Constants.TestSourceName}_{Constants.TestSourceType}"), Is.True);
            Assert.That(showResult.StdOut.Contains("id: AppInstallerTest.TestPackageExport"), Is.True);
            Assert.That(showResult.StdOut.Contains($"source: {Constants.TestSourceName}"), Is.True);
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
            Assert.That(File.Exists(exportFile), Is.True);

            // Check exported file is readable and validate content
            var showResult = TestCommon.RunAICLICommand(ShowCommand, $"-f {exportFile}");
            Assert.That(showResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(showResult.StdOut.Contains("Microsoft.WinGet.Dev/Source"), Is.True);
            Assert.That(showResult.StdOut.Contains($"[{Constants.TestSourceName}_{Constants.TestSourceType}]"), Is.True);
            Assert.That(showResult.StdOut.Contains($"type: {Constants.TestSourceType}"), Is.True);
            Assert.That(showResult.StdOut.Contains($"argument: {Constants.TestSourceUrl}"), Is.True);
            Assert.That(showResult.StdOut.Contains($"name: {Constants.TestSourceName}"), Is.True);

            Assert.That(showResult.StdOut.Contains("Microsoft.WinGet.Dev/Package"), Is.True);
            Assert.That(showResult.StdOut.Contains($"[{Constants.TestSourceName}_AppInstallerTest.TestPackageExport]"), Is.True);
            Assert.That(showResult.StdOut.Contains($"Dependencies: {Constants.TestSourceName}_{Constants.TestSourceType}"), Is.True);
            Assert.That(showResult.StdOut.Contains("id: AppInstallerTest.TestPackageExport"), Is.True);
            Assert.That(showResult.StdOut.Contains($"source: {Constants.TestSourceName}"), Is.True);

            Assert.That(showResult.StdOut.Contains("AppInstallerTest/TestResource"), Is.True);
            Assert.That(showResult.StdOut.Contains($"Dependencies: {Constants.TestSourceName}_AppInstallerTest.TestPackageExport"), Is.True);
            Assert.That(showResult.StdOut.Contains("data: TestData"), Is.True);
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
            Assert.That(File.Exists(exportFile), Is.True);

            // Check exported file is readable and validate content
            var showResult = TestCommon.RunAICLICommand(ShowCommand, $"-f {exportFile}");
            Assert.That(showResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(showResult.StdOut.Contains("Microsoft.WinGet.Dev/Source"), Is.True);
            Assert.That(showResult.StdOut.Contains($"[{Constants.TestSourceName}_{Constants.TestSourceType}]"), Is.True);
            Assert.That(showResult.StdOut.Contains($"type: {Constants.TestSourceType}"), Is.True);
            Assert.That(showResult.StdOut.Contains($"argument: {Constants.TestSourceUrl}"), Is.True);
            Assert.That(showResult.StdOut.Contains($"name: {Constants.TestSourceName}"), Is.True);

            Assert.That(showResult.StdOut.Contains("Microsoft.WinGet.Dev/Package"), Is.True);
            Assert.That(showResult.StdOut.Contains($"[{Constants.TestSourceName}_AppInstallerTest.TestPackageExport]"), Is.True);
            Assert.That(showResult.StdOut.Contains($"Dependencies: {Constants.TestSourceName}_{Constants.TestSourceType}"), Is.True);
            Assert.That(showResult.StdOut.Contains("id: AppInstallerTest.TestPackageExport"), Is.True);
            Assert.That(showResult.StdOut.Contains($"source: {Constants.TestSourceName}"), Is.True);
            Assert.That(showResult.StdOut.Contains("version: 1.0.0.0"), Is.True);
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
            Assert.That(File.Exists(exportFile), Is.True);

            // Check exported file is readable and validate content
            var showResult = TestCommon.RunAICLICommand(ShowCommand, $"-f {exportFile}", timeOut: 1200000);
            Assert.That(showResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            Assert.That(showResult.StdOut.Contains("Microsoft.PowerShell"), Is.True);

            Assert.That(showResult.StdOut.Contains("Microsoft.WinGet.Dev/UserSettingsFile"), Is.True);
            Assert.That(showResult.StdOut.Contains("Microsoft.WinGet.Dev/AdminSettings"), Is.True);
            Assert.That(showResult.StdOut.Contains("Microsoft.Windows.Settings/WindowsSettings"), Is.True);

            Assert.That(showResult.StdOut.Contains("Microsoft.WinGet.Dev/Source"), Is.True);
            Assert.That(showResult.StdOut.Contains($"[{Constants.TestSourceName}_{Constants.TestSourceType}]"), Is.True);
            Assert.That(showResult.StdOut.Contains($"type: {Constants.TestSourceType}"), Is.True);
            Assert.That(showResult.StdOut.Contains($"argument: {Constants.TestSourceUrl}"), Is.True);
            Assert.That(showResult.StdOut.Contains($"name: {Constants.TestSourceName}"), Is.True);

            Assert.That(showResult.StdOut.Contains("Microsoft.WinGet.Dev/Package"), Is.True);
            Assert.That(showResult.StdOut.Contains($"[{Constants.TestSourceName}_AppInstallerTest.TestPackageExport]"), Is.True);
            Assert.That(showResult.StdOut.Contains($"Dependencies: {Constants.TestSourceName}_{Constants.TestSourceType}"), Is.True);
            Assert.That(showResult.StdOut.Contains("id: AppInstallerTest.TestPackageExport"), Is.True);
            Assert.That(showResult.StdOut.Contains($"source: {Constants.TestSourceName}"), Is.True);

            Assert.That(showResult.StdOut.Contains("AppInstallerTest/TestResource"), Is.True);
            Assert.That(showResult.StdOut.Contains($"Dependencies: {Constants.TestSourceName}_AppInstallerTest.TestPackageExport"), Is.True);
            Assert.That(showResult.StdOut.Contains("data: TestData"), Is.True);

            Assert.That(showResult.StdOut.Contains("AppInstallerTest/TestResource_SubDirectory"), Is.True);
            Assert.That(showResult.StdOut.Contains($"Dependencies: {Constants.TestSourceName}_AppInstallerTest.TestPackageExport"), Is.True);
            Assert.That(showResult.StdOut.Contains("data: TestData"), Is.True);
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
