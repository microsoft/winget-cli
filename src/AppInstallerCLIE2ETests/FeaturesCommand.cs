// -----------------------------------------------------------------------------
// <copyright file="FeaturesCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    /// <summary>
    /// Features command tests.
    /// </summary>
    public class FeaturesCommand : BaseCommand
    {
        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            WinGetSettingsHelper.InitializeAllFeatures(false);
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [TearDown]
        public void TearDown()
        {
            WinGetSettingsHelper.InitializeAllFeatures(false);
        }

        /// <summary>
        /// Tests winget features.
        /// </summary>
        [Test]
        public void DisplayFeatures()
        {
            var result = TestCommon.RunAICLICommand("features", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Direct MSI Installation"));
            Assert.False(result.StdOut.Contains("Enabled"));
        }

        /// <summary>
        /// Tests enabled winget features.
        /// </summary>
        [Test]
        public void EnableExperimentalFeatures()
        {
            WinGetSettingsHelper.ConfigureFeature("experimentalArg", true);
            WinGetSettingsHelper.ConfigureFeature("experimentalCmd", true);
            WinGetSettingsHelper.ConfigureFeature("directMSI", true);
            WinGetSettingsHelper.ConfigureFeature("uninstallPreviousArgument", true);
            WinGetSettingsHelper.ConfigureFeature("windowsFeature", true);
            var result = TestCommon.RunAICLICommand("features", string.Empty);
            Assert.True(result.StdOut.Contains("Enabled"));
        }
    }
}
