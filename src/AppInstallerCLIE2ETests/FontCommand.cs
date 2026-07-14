// -----------------------------------------------------------------------------
// <copyright file="FontCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test font command.
    /// </summary>
    public class FontCommand : BaseCommand
    {
        /// <summary>
        /// One time set up.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            WinGetSettingsHelper.ConfigureFeature("fonts", true);
        }

        /// <summary>
        /// Test install a font with user scope.
        /// </summary>
        [Test]
        public void InstallFont_UserScope()
        {
            var fontPackageName = "AppInstallerTest.TestFont";
            var fontPackageVersion = "1.0.0.0";
            var installResult = TestCommon.RunAICLICommand("install", fontPackageName);
            Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(installResult.StdOut, Does.Contain("Successfully installed"));
            TestCommon.VerifyFontPackage(fontPackageName, fontPackageVersion, TestCommon.Scope.User);

            var uninstallResult = TestCommon.RunAICLICommand("uninstall", fontPackageName);
            Assert.That(uninstallResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            TestCommon.VerifyFontPackage(fontPackageName, fontPackageVersion, TestCommon.Scope.User, false);
        }

        /// <summary>
        /// Test install a font with machine scope.
        /// </summary>
        [Test]
        public void InstallFont_MachineScope()
        {
            var fontPackageName = "AppInstallerTest.TestFont";
            var fontPackageVersion = "1.0.0.0";
            var result = TestCommon.RunAICLICommand("install", $"{fontPackageName} --scope Machine");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            TestCommon.VerifyFontPackage(fontPackageName, fontPackageVersion, TestCommon.Scope.Machine);

            var uninstallResult = TestCommon.RunAICLICommand("uninstall", fontPackageName);
            Assert.That(uninstallResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            TestCommon.VerifyFontPackage(fontPackageName, fontPackageVersion, TestCommon.Scope.Machine, false);
        }

        /// <summary>
        /// Test install an invalid font file.
        /// </summary>
        [Test]
        public void InstallInvalidFont()
        {
            var result = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestInvalidFont");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_FONT_FILE_NOT_SUPPORTED));
            Assert.That(result.StdOut, Does.Contain("One or more fonts in the font package is not supported and cannot be installed."));
        }
    }
}
