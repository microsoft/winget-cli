// -----------------------------------------------------------------------------
// <copyright file="Pinning.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using static AppInstallerCLIE2ETests.TestCommon;

    /// <summary>
    /// Test upgrading pinned packages.
    /// </summary>
    public class Pinning : BaseCommand
    {
        /// <summary>
        /// Setup done once before all the tests here.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            WinGetSettingsHelper.ConfigureFeature("pinning", true);
        }

        /// <summary>
        /// Set up for all tests.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            // All tests use TestExeInstaller; try to clean it up for failure cases,
            // then install the base version for pinning
            TestCommon.RunAICLICommand("uninstall", "AppInstallerTest.TestExeInstaller");
            TestCommon.RunAICLICommand("install", "AppInstallerTest.TestExeInstaller -v 1.0.0.0");
            TestCommon.RunAICLICommand("pin remove", "AppInstallerTest.TestExeInstaller");
        }

        /// <summary>
        /// Clean up done after all the tests here.
        /// </summary>
        [OneTimeTearDown]
        public void OneTimeTearDown()
        {
            TestCommon.RunAICLICommand("pin remove", "AppInstallerTest.TestExeInstaller");
            TestCommon.RunAICLICommand("uninstall", "AppInstallerTest.TestExeInstaller");
        }

        // All tests do roughly the same with different types of pins:
        // * Check that the available version shown by list is the latest
        // * Check that the available version shown by upgrade is appropriate for the pin,
        //   including checks with flags to include pinned.
        // * Check that an upgrade installs the right version

        /// <summary>
        /// Tests upgrading a package when there are no pins on it.
        /// </summary>
        [Test]
        public void UpgradeWithNoPins()
        {
            RunCommandResult result;

            result = TestCommon.RunAICLICommand("list", "AppInstallerTest.TestExeInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(result.StdOut.Contains("2.0.0.0"), "List shows the latest available version");

            result = TestCommon.RunAICLICommand("upgrade", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(result.StdOut.Contains("2.0.0.0"), "The latest upgrade-able version is the same if there are no pins");
        }

        /// <summary>
        /// Tests upgrading a package when it has a pinning pin.
        /// </summary>
        [Test]
        public void UpgradeWithPinningPin()
        {
            RunCommandResult result;

            result = TestCommon.RunAICLICommand("pin add", "AppInstallerTest.TestExeInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            result = TestCommon.RunAICLICommand("list", "AppInstallerTest.TestExeInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(result.StdOut.Contains("2.0.0.0"), "List shows the latest available version");

            result = TestCommon.RunAICLICommand("upgrade", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsFalse(result.StdOut.Contains("2.0.0.0"), "Pin hides latest available version");
            Assert.IsTrue(result.StdOut.Contains("package(s) have pins that prevent upgrade"));

            result = TestCommon.RunAICLICommand("upgrade", "--all");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            result = TestCommon.RunAICLICommand("upgrade", "--include-pinned");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(result.StdOut.Contains("2.0.0.0"), "Argument makes available version show up");

            result = TestCommon.RunAICLICommand("upgrade", "--all --include-pinned");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode, "Upgrade succeeds");
        }

        /// <summary>
        /// Tests upgrading a package when it has a gating pin that allows updating to another version.
        /// </summary>
        [Test]
        public void UpgradeWithGatingPin()
        {
            RunCommandResult result;

            var pinResult = TestCommon.RunAICLICommand("pin add", "AppInstallerTest.TestExeInstaller --version 1.0.*");
            Assert.AreEqual(Constants.ErrorCode.S_OK, pinResult.ExitCode);

            result = TestCommon.RunAICLICommand("list", "AppInstallerTest.TestExeInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(result.StdOut.Contains("2.0.0.0"), "List shows the latest available version");

            result = TestCommon.RunAICLICommand("upgrade", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsFalse(result.StdOut.Contains("2.0.0.0"), "Pin hides latest available version");
            Assert.IsTrue(result.StdOut.Contains("1.0.1.0"), "Version matching pin gated version shows up");

            result = TestCommon.RunAICLICommand("upgrade", "--all");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode, "Upgrade succeeds");
        }

        /// <summary>
        /// Tests upgrading a package when it has a gating pin that blocks all other versions.
        /// </summary>
        [Test]
        public void UpgradeWithGatingPinToCurrent()
        {
            RunCommandResult result;

            result = TestCommon.RunAICLICommand("pin add", "AppInstallerTest.TestExeInstaller --version 1.0.0.*");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            result = TestCommon.RunAICLICommand("list", "AppInstallerTest.TestExeInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(result.StdOut.Contains("2.0.0.0"), "List shows the latest available version");

            result = TestCommon.RunAICLICommand("upgrade", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsFalse(result.StdOut.Contains("2.0.0.0"), "Pin hides latest available version");
            Assert.IsTrue(result.StdOut.Contains("package(s) have pins that prevent upgrade"));

            result = TestCommon.RunAICLICommand("upgrade", "AppInstallerTest.TestExeInstaller");
            Assert.AreEqual(Constants.ErrorCode.ERROR_UPDATE_NOT_APPLICABLE, result.ExitCode, "No upgrades available");
        }

        /// <summary>
        /// Tests upgrading a package when it has a blocking pin.
        /// </summary>
        [Test]
        public void UpgradeWithBlockingPin()
        {
            RunCommandResult result;

            var pinResult = TestCommon.RunAICLICommand("pin add", "AppInstallerTest.TestExeInstaller --blocking");
            Assert.AreEqual(Constants.ErrorCode.S_OK, pinResult.ExitCode);

            result = TestCommon.RunAICLICommand("list", "AppInstallerTest.TestExeInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(result.StdOut.Contains("2.0.0.0"), "List shows the latest available version");

            result = TestCommon.RunAICLICommand("upgrade", string.Empty);
            Assert.IsFalse(result.StdOut.Contains("2.0.0.0"), "Pin hides latest available version");
            Assert.IsTrue(result.StdOut.Contains("package(s) have pins that prevent upgrade"));

            result = TestCommon.RunAICLICommand("upgrade", "AppInstallerTest.TestExeInstaller");
            Assert.AreEqual(Constants.ErrorCode.ERROR_UPDATE_NOT_APPLICABLE, result.ExitCode, "No upgrades available");
        }
    }
}
