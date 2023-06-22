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
            var result = TestCommon.RunAICLICommand("download", $"{Constants.ExeInstallerPackageId} --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(TestCommon.VerifyInstallerDownload(downloadDir, Constants.AppInstallerTestExeInstallerExe));
        }

        /// <summary>
        /// Downloads the test installer using the user scope argument.
        /// </summary>
        [Test]
        public void DownloadWithUserScope()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestMultipleInstallers --scope user --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(TestCommon.VerifyInstallerDownload(downloadDir, Constants.AppInstallerTestExeInstallerExe));
        }

        /// <summary>
        /// Downloads the test installer using the machine scope argument.
        /// </summary>
        [Test]
        public void DownloadWithMachineScope()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestMultipleInstallers --scope machine --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(TestCommon.VerifyInstallerDownload(downloadDir, Constants.AppInstallerTestMsiInstallerMsi));
        }

        /// <summary>
        /// Downloads the test installer using the 'zip' installer type argument. Verifies that base installer types such as 'zip' are still supported.
        /// </summary>
        [Test]
        public void DownloadWithZipInstallerTypeArg()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestMultipleInstallers --installer-type zip --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(TestCommon.VerifyInstallerDownload(downloadDir, Constants.AppInstallerTestZipInstallerZip));
        }

        /// <summary>
        /// Downloads the test installer using the installer type argument.
        /// </summary>
        [Test]
        public void DownloadWithInstallerTypeArg()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestMultipleInstallers --installer-type msi --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(TestCommon.VerifyInstallerDownload(downloadDir, Constants.AppInstallerTestMsiInstallerMsi));
        }

        /// <summary>
        /// Downloads the test installer using the architecture argument.
        /// </summary>
        [Test]
        public void DownloadWithArchitectureArg()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestMultipleInstallers --architecture x86 --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(TestCommon.VerifyInstallerDownload(downloadDir, Constants.AppInstallerTestMsiInstallerMsi));
        }

        /// <summary>
        /// Downlods the test installer with a hash mismatch.
        /// </summary>
        [Test]
        public void DownloadWithHashMismatch()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var errorResult = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestExeSha256Mismatch --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, errorResult.ExitCode);
        }
    }
}