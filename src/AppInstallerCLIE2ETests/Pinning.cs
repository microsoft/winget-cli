// -----------------------------------------------------------------------------
// <copyright file="Pinning.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;
    using static AppInstallerCLIE2ETests.Helpers.TestCommon;

    /// <summary>
    /// Test upgrading pinned packages.
    /// </summary>
    public class Pinning : BaseCommand
    {
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("2.0.0.0"), "List shows the latest available version");

            result = TestCommon.RunAICLICommand("upgrade", string.Empty);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("2.0.0.0"), "The latest upgrade-able version is the same if there are no pins");
        }

        /// <summary>
        /// Tests upgrading a package when it has a pinning pin.
        /// </summary>
        [Test]
        public void UpgradeWithPinningPin()
        {
            RunCommandResult result;
            string installDir = Path.GetTempPath();

            // The base version of this app does not log /Version, but it still includes the version number in the log file name.
            Assert.That(TestCommon.VerifyTestExeInstalled(installDir, "1.0.0.0"), Is.True, "Base version installed");

            result = TestCommon.RunAICLICommand("pin add", "AppInstallerTest.TestExeInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            result = TestCommon.RunAICLICommand("list", "AppInstallerTest.TestExeInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("2.0.0.0"), "List shows the latest available version");

            result = TestCommon.RunAICLICommand("upgrade", string.Empty);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Not.Contain("2.0.0.0"), "Pin hides latest available version");
            Assert.That(result.StdOut, Does.Contain("package(s) have pins that prevent upgrade"));

            result = TestCommon.RunAICLICommand("upgrade", "--all");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK), "Upgrade succeeds with nothing to upgrade");

            Assert.That(TestCommon.VerifyTestExeInstalled(installDir, "1.0.0.0"), Is.True, "No newer version installed");

            result = TestCommon.RunAICLICommand("upgrade", "--include-pinned");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("2.0.0.0"), "Argument makes available version show up");

            result = TestCommon.RunAICLICommand("upgrade", "--all --include-pinned");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK), "Upgrade succeeds");
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/Version 2.0.0.0"), Is.True);
        }

        /// <summary>
        /// Tests upgrading a package when it has a gating pin that allows updating to another version.
        /// </summary>
        [Test]
        public void UpgradeWithGatingPin()
        {
            RunCommandResult result;
            string installDir = Path.GetTempPath();

            var pinResult = TestCommon.RunAICLICommand("pin add", "AppInstallerTest.TestExeInstaller --version 1.0.*");
            Assert.That(pinResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            result = TestCommon.RunAICLICommand("list", "AppInstallerTest.TestExeInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("2.0.0.0"), "List shows the latest available version");

            result = TestCommon.RunAICLICommand("upgrade", string.Empty);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Not.Contain("2.0.0.0"), "Pin hides latest available version");
            Assert.That(result.StdOut, Does.Contain("1.0.1.0"), "Version matching pin gated version shows up");

            result = TestCommon.RunAICLICommand("upgrade", "--all");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK), "Upgrade succeeds");
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/Version 1.0.1.0"), Is.True);
        }

        /// <summary>
        /// Tests upgrading a package when it has a gating pin that blocks all other versions.
        /// </summary>
        [Test]
        public void UpgradeWithGatingPinToCurrent()
        {
            RunCommandResult result;

            result = TestCommon.RunAICLICommand("pin add", "AppInstallerTest.TestExeInstaller --version 1.0.0.*");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            result = TestCommon.RunAICLICommand("list", "AppInstallerTest.TestExeInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("2.0.0.0"), "List shows the latest available version");

            result = TestCommon.RunAICLICommand("upgrade", string.Empty);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Not.Contain("2.0.0.0"), "Pin hides latest available version");
            Assert.That(result.StdOut, Does.Contain("package(s) have pins that prevent upgrade"));

            result = TestCommon.RunAICLICommand("upgrade", "AppInstallerTest.TestExeInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_PACKAGE_IS_PINNED), "No upgrades available due to pin");
        }

        /// <summary>
        /// Tests upgrading a package when it has a blocking pin.
        /// </summary>
        [Test]
        public void UpgradeWithBlockingPin()
        {
            RunCommandResult result;

            var pinResult = TestCommon.RunAICLICommand("pin add", "AppInstallerTest.TestExeInstaller --blocking");
            Assert.That(pinResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            result = TestCommon.RunAICLICommand("list", "AppInstallerTest.TestExeInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("2.0.0.0"), "List shows the latest available version");

            result = TestCommon.RunAICLICommand("upgrade", string.Empty);
            Assert.That(result.StdOut, Does.Not.Contain("2.0.0.0"), "Pin hides latest available version");
            Assert.That(result.StdOut, Does.Contain("package(s) have pins that prevent upgrade"));

            result = TestCommon.RunAICLICommand("upgrade", "AppInstallerTest.TestExeInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_PACKAGE_IS_PINNED), "No upgrades available due to pin");
        }

        /// <summary>
        /// Tests upgrading a package when it has a pinning pin and the --force flag is used.
        /// </summary>
        [Test]
        public void ForceUpgradeWithPinningPin()
        {
            RunCommandResult result;
            string installDir = Path.GetTempPath();

            result = TestCommon.RunAICLICommand("pin add", "AppInstallerTest.TestExeInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            result = TestCommon.RunAICLICommand("upgrade", "--force");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("2.0.0.0"), "--force argument shows latest version");

            result = TestCommon.RunAICLICommand("upgrade", "--all --force");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/Version 2.0.0.0"), Is.True, "--force argument installs last version despite pin");
        }

        /// <summary>
        /// Tests upgrading a package when it has a gating pin and the --force flag is used.
        /// </summary>
        [Test]
        public void ForceUpgradeWithGatingPin()
        {
            RunCommandResult result;
            string installDir = Path.GetTempPath();

            var pinResult = TestCommon.RunAICLICommand("pin add", "AppInstallerTest.TestExeInstaller --version 1.0.*");
            Assert.That(pinResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            result = TestCommon.RunAICLICommand("upgrade", "--force");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("2.0.0.0"), "--force argument shows latest version");

            result = TestCommon.RunAICLICommand("upgrade", "--all --force");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK), "Upgrade succeeds");
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/Version 2.0.0.0"), Is.True);
        }

        /// <summary>
        /// Tests upgrading a package when it has a blocking pin and the --force flag is used.
        /// </summary>
        [Test]
        public void ForceUpgradeWithBlockingPin()
        {
            RunCommandResult result;
            string installDir = Path.GetTempPath();

            var pinResult = TestCommon.RunAICLICommand("pin add", "AppInstallerTest.TestExeInstaller --blocking");
            Assert.That(pinResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            result = TestCommon.RunAICLICommand("upgrade", "--force");
            Assert.That(result.StdOut, Does.Contain("2.0.0.0"), "--force argument shows latest version");

            result = TestCommon.RunAICLICommand("upgrade", "--all --force");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK), "Upgrade succeeds");
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(installDir, "/Version 2.0.0.0"), Is.True);
        }
    }
}
