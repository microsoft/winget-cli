// -----------------------------------------------------------------------------
// <copyright file="DownloadCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using NUnit.Framework;

    /// <summary>
    /// Test download command.
    /// </summary>
    public class DownloadCommand : BaseCommand
    {
        /// <summary>
        /// One time setup.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            WinGetSettingsHelper.ConfigureFeature("download", true);
        }

        /// <summary>
        /// Downloads the test installer to the default downloads directory.
        /// </summary>
        [Test]
        public void DownloadToDefaultDirectory()
        {
            var packageVersion = "2.0.0.0";
            var result = TestCommon.RunAICLICommand("download", $"{Constants.ExeInstallerPackageId} --version {packageVersion}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            string downloadDir = Path.Combine(TestCommon.GetDefaultDownloadDirectory(), $"{Constants.ExeInstallerPackageId}.{packageVersion}");
            Assert.True(TestCommon.VerifyInstallerDownload(downloadDir, Constants.AppInstallerTestExeInstallerExe));
        }

        /// <summary>
        /// Downloads the test installer to a specified directory.
        /// </summary>
        [Test]
        public void DownloadToDirectory()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestExeInstaller --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(TestCommon.VerifyInstallerDownload(downloadDir, Constants.AppInstallerTestExeInstallerExe));
        }

        [Test]
        public void DownloadWithScopeArg()
        {

        }

        [Test]
        public void DownloadWithInstallerTypeArg()
        {

        }

        [Test]
        public void DownloadWithArchitectureArg()
        {

        }

        [Test]
        public void DownloadWithHashMismatch()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var errorResult = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestExeSha256Mismatch --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, errorResult.ExitCode);
        }

        // These are the scenarios that I want to cover in the E2E tests
        // Create a manifest that has 2 installers of exe, 2 installers of msi, different scope and different args as well as a zip installer so that we can verify zip still works.
        // Create a manifest that has a dependency and download those installers as well to a specific test folder.
        // Do we need to clean up folder afterwards?
        // Refactor and Create PR?
        // Run Dependency Tests
        // Add COM tests as well.

        // Download manifest with dependencies
        // Download manifest with dependencies but including the skip dependencies argument
        // Download specific installer using scope
        // Download specific installer using arch
        // Download specific installer using installertype
        // Download specific installer using locale

        // Dependency graph needs a unit test for verifying that it produces installed as well, possibly.
    }
}