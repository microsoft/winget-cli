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
            var result = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestFont");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyFontPackage(Constants.TestFontSubKeyName, Constants.FontFileName, TestCommon.Scope.User);
        }

        /// <summary>
        /// Test install a font with machine scope.
        /// </summary>
        [Test]
        public void InstallFont_MachineScope()
        {
            var result = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestFont --scope Machine");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyFontPackage(Constants.TestFontSubKeyName, Constants.FontFileName, TestCommon.Scope.Machine);
        }

        /// <summary>
        /// Test install an invalid font file.
        /// </summary>
        [Test]
        public void InstallInvalidFont()
        {
            var result = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestInvalidFont");
            Assert.AreEqual(Constants.ErrorCode.ERROR_FONT_FILE_NOT_SUPPORTED, result.ExitCode);
            Assert.True(result.StdOut.Contains("The font file is not supported and cannot be installed."));
        }
    }
}
