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
            WinGetSettingsHelper.ConfigureFeature("dependencies", true);
            WinGetSettingsHelper.ConfigureFeature("windowsFeature", true);
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
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            Assert.True(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/execustom"));

            int actualCheckpointsCount = Directory.GetFiles(checkpointsDir).Length;

            // The checkpoints count should not change as the index file should be cleaned up after a successful install.
            Assert.AreEqual(initialCheckpointsCount, actualCheckpointsCount);
        }

        /// <summary>
        /// Installs a test exe installer and verifies that the checkpoint index is cleaned up.
        /// </summary>
        [Test]
        public void ResumeIndex()
        {
            var checkpointsDir = TestCommon.GetCheckpointsDirectory();

            // If the checkpoints directory does not yet exist, set to 0. The directory should be created when the command is invoked.
            int initialCheckpointsCount = Directory.Exists(checkpointsDir) ? Directory.GetFiles(checkpointsDir).Length : 0;

            var installResult = TestCommon.RunAICLICommand("install", "--id AppInstallerTest.WindowsFeature");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALL_MISSING_DEPENDENCY, installResult.ExitCode);
            Assert.True(installResult.StdOut.Contains("The feature [invalidFeature] was not found."));

            int actualCheckpointsCount = Directory.GetFiles(checkpointsDir).Length;

            // One new index file should be created after running the install command.
            Assert.AreEqual(initialCheckpointsCount + 1, actualCheckpointsCount);

            var checkpointsDirectoryInfo = new DirectoryInfo(checkpointsDir);

            var indexFile = checkpointsDirectoryInfo.GetFiles()
             .OrderByDescending(f => f.LastWriteTime)
             .First();

            var indexFileName = Path.GetFileNameWithoutExtension(indexFile.Name);

            // Resume output should be the same as the install result.
            var resumeResult = TestCommon.RunAICLICommand("resume", $"-g {indexFileName}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALL_MISSING_DEPENDENCY, resumeResult.ExitCode);
            Assert.True(resumeResult.StdOut.Contains("The feature [invalidFeature] was not found."));
        }
    }
}