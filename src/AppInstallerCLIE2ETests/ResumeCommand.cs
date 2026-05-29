// -----------------------------------------------------------------------------
// <copyright file="ResumeCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using System.Linq;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test download command.
    /// </summary>
    public class ResumeCommand : BaseCommand
    {
        /// <summary>
        /// One time setup.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            WinGetSettingsHelper.ConfigureFeature("resume", true);
        }

        /// <summary>
        /// One time teardown.
        /// </summary>
        [OneTimeTearDown]
        public void OneTimeTearDown()
        {
            WinGetSettingsHelper.ConfigureFeature("resume", false);
        }

        /// <summary>
        /// Installs a test exe installer and verifies that the checkpoint index is cleaned up.
        /// </summary>
        [Test]
        public void InstallExe_VerifyIndexDoesNotExist()
        {
            var checkpointsDir = TestCommon.GetCheckpointsDirectory();

            // If the checkpoints directory does not yet exist, set to 0. The directory should be created when the command is invoked.
            int initialCheckpointsCount = Directory.Exists(checkpointsDir) ? Directory.GetFiles(checkpointsDir).Length : 0;

            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {installDir}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Successfully installed"), Is.True);
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"), Is.True);

            int actualCheckpointsCount = Directory.GetFiles(checkpointsDir).Length;

            // The checkpoints count should not change as the index file should be cleaned up after a successful install.
            Assert.That(actualCheckpointsCount, Is.EqualTo(initialCheckpointsCount));
        }

        /// <summary>
        /// Verifies that an error message is shown when a resume id does not exist.
        /// </summary>
        [Test]
        public void ResumeIdNotFound()
        {
            var resumeResult = TestCommon.RunAICLICommand("resume", "-g invalidResumeId");
            Assert.That(resumeResult.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_RESUME_ID_NOT_FOUND));
        }

        /// <summary>
        /// Test install a package that returns REBOOT_REQUIRED_TO_FINISH and verify that the checkpoint database is properly cleaned up.
        /// </summary>
        [Test]
        public void InstallRequiresRebootToFinish()
        {
            var checkpointsDir = TestCommon.GetCheckpointsDirectory();
            int initialCheckpointsCount = Directory.Exists(checkpointsDir) ? Directory.GetDirectories(checkpointsDir).Length : 0;

            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestRebootRequired --custom \"/ExitCode 9\" -l {installDir}");

            // REBOOT_REQUIRED_TO_FINISH is treated as a success.
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Restart your PC to finish installation."), Is.True);
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir), Is.True);

            int actualCheckpointsCount = Directory.GetDirectories(checkpointsDir).Length;

            // Checkpoint database should be cleaned up since resume is not needed to complete installation.
            Assert.That(actualCheckpointsCount, Is.EqualTo(initialCheckpointsCount));
        }

        /// <summary>
        /// Test install a package that returns REBOOT_REQUIRED_FOR_INSTALL and verify that resume command can be called successfully.
        /// </summary>
        [Test]
        public void InstallRequiresRebootForInstall()
        {
            var checkpointsDir = TestCommon.GetCheckpointsDirectory();

            int initialCheckpointsCount = Directory.Exists(checkpointsDir) ? Directory.GetDirectories(checkpointsDir).Length : 0;

            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"TestRebootRequired --custom \"/ExitCode 10\" -l {installDir}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_INSTALL_REBOOT_REQUIRED_FOR_INSTALL));
            Assert.That(result.StdOut.Contains("Your PC will restart to finish installation."), Is.True);
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir), Is.True);

            int actualCheckpointsCount = Directory.GetDirectories(checkpointsDir).Length;
            Assert.That(actualCheckpointsCount, Is.EqualTo(initialCheckpointsCount + 1));

            var checkpointsDirectoryInfo = new DirectoryInfo(checkpointsDir);

            var checkpoint = checkpointsDirectoryInfo.GetDirectories()
             .OrderByDescending(f => f.LastWriteTime)
             .First();

            // Resume output should be the same as the install result.
            var resumeResult = TestCommon.RunAICLICommand("resume", $"-g {checkpoint.Name}");
            Assert.That(resumeResult.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_INSTALL_REBOOT_REQUIRED_FOR_INSTALL));
            Assert.That(resumeResult.StdOut.Contains("Your PC will restart to finish installation."), Is.True);
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir), Is.True);
        }
    }
}