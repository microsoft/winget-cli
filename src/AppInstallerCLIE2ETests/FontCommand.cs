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

            // TODO: Remove once font manifests can be installed from a source.
            TestCommon.RunAICLICommand("settings", "--enable LocalManifestFiles");
        }

        /// <summary>
        /// One time tear down.
        /// </summary>
        [OneTimeTearDown]
        public void OneTimeTearDown()
        {
            // TODO: Remove once font manifests can be installed from a source.
            TestCommon.RunAICLICommand("settings", "--disable LocalManifestFiles");
        }

        /// <summary>
        /// Test install a font with user scope.
        /// </summary>
        [Test]
        public void InstallFont()
        {
            var result = TestCommon.RunAICLICommand("font install", $"-m {TestCommon.GetTestDataFile(@"Manifests\TestFont.yaml")}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyFontPackage(Constants.TestFontSubKeyName, Constants.FontFileName);
        }

        /// <summary>
        /// Test install a font with machine scope.
        /// </summary>
        [Test]
        public void InstallFont_MachineScope()
        {
            var result = TestCommon.RunAICLICommand("font install", $"-m {TestCommon.GetTestDataFile(@"Manifests\TestFont.yaml")} --scope Machine");
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
            var result = TestCommon.RunAICLICommand("font install", $"-m {TestCommon.GetTestDataFile(@"Manifests\TestInvalidFont.yaml")} --scope Machine");
            Assert.AreEqual(Constants.ErrorCode.ERROR_FONT_FILE_NOT_SUPPORTED, result.ExitCode);
            Assert.True(result.StdOut.Contains("The font file is not supported and cannot be installed."));
        }
    }
}
