// -----------------------------------------------------------------------------
// <copyright file="Settings.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Tests user settings behavior.
    /// </summary>
    public class Settings
    {
        /// <summary>
        /// Reset settings before these tests run.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            WinGetSettingsHelper.InitializeWingetSettings();
        }

        /// <summary>
        /// Reset settings after these tests complete.
        /// </summary>
        [OneTimeTearDown]
        public void OneTimeTearDown()
        {
            WinGetSettingsHelper.InitializeWingetSettings();
        }

        /// <summary>
        /// Verifies that changing output.locale changes help output.
        /// </summary>
        [Test]
        public void OutputLocaleChangesHelpOutput()
        {
            WinGetSettingsHelper.ConfigureOutputLocale("en-US");
            var english = RunHelpCommand();

            WinGetSettingsHelper.ConfigureOutputLocale("ru-RU");
            var russian = RunHelpCommand();

            Assert.That(russian.StdOut, Is.Not.EqualTo(english.StdOut));

            WinGetSettingsHelper.ConfigureOutputLocale("en-US");
            var englishAgain = RunHelpCommand();
            Assert.That(englishAgain.StdOut, Is.EqualTo(english.StdOut));
        }

        // Shared command helper for settings tests that validate global CLI output.
        private static TestCommon.RunCommandResult RunHelpCommand()
        {
            var result = TestCommon.RunAICLICommand(string.Empty, "-?");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            return result;
        }
    }
}
