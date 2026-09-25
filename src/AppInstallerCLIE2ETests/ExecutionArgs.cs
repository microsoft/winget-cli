// -----------------------------------------------------------------------------
// <copyright file="ExecutionArgs.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Tests the behavior of command line arguments.
    /// </summary>
    public class ExecutionArgs : BaseCommand
    {
        private const string GermanSystemArchitecture = "Systemarchitektur:";
        private const string GermanKeyDirectoriesHeader = "WinGet-Verzeichnisse";
        private const string EnglishSystemArchitecture = "System Architecture:";
        private const string EnglishKeyDirectoriesHeader = "WinGet Directories";
        private const string RussianKeyDirectoriesHeader = "Каталоги WinGet";

        /// <summary>
        /// Reset settings before these tests run.
        /// </summary>
        [OneTimeSetUp]
        public void ExecutionArgsSetup()
        {
            WinGetSettingsHelper.InitializeWingetSettings();
        }

        /// <summary>
        /// Reset settings after these tests complete.
        /// </summary>
        [OneTimeTearDown]
        public void ExecutionArgsTeardown()
        {
            WinGetSettingsHelper.InitializeWingetSettings();
        }

        /// <summary>
        /// Verifies that --output-locale alone changes the language of winget's own output.
        /// </summary>
        [Test]
        public void OutputLocaleArgumentChangesOutputLanguage()
        {
            var german = TestCommon.RunAICLICommand(string.Empty, "--info --output-locale de-DE");
            Assert.That(german.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(german.StdOut, Does.Contain(GermanSystemArchitecture));
            Assert.That(german.StdOut, Does.Contain(GermanKeyDirectoriesHeader));

            var english = TestCommon.RunAICLICommand(string.Empty, "--info --output-locale en-US");
            Assert.That(english.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(english.StdOut, Does.Contain(EnglishSystemArchitecture));
            Assert.That(english.StdOut, Does.Contain(EnglishKeyDirectoriesHeader));
            Assert.That(english.StdOut, Does.Not.Contain(GermanKeyDirectoriesHeader));
        }

        /// <summary>
        /// Verifies that --output-locale takes precedence over the output.locale user setting.
        /// </summary>
        [Test]
        public void OutputLocaleArgumentTakesPrecedenceOverSetting()
        {
            try
            {
                WinGetSettingsHelper.ConfigureOutputLocale("ru-RU");

                // Without the argument, the setting is used.
                var fromSetting = TestCommon.RunAICLICommand(string.Empty, "--info");
                Assert.That(fromSetting.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                Assert.That(fromSetting.StdOut, Does.Contain(RussianKeyDirectoriesHeader));

                // With the argument, the argument wins.
                var fromArgument = TestCommon.RunAICLICommand(string.Empty, "--info --output-locale de-DE");
                Assert.That(fromArgument.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                Assert.That(fromArgument.StdOut, Does.Contain(GermanSystemArchitecture));
                Assert.That(fromArgument.StdOut, Does.Contain(GermanKeyDirectoriesHeader));
                Assert.That(fromArgument.StdOut, Does.Not.Contain(RussianKeyDirectoriesHeader));
            }
            finally
            {
                WinGetSettingsHelper.ConfigureOutputLocale(null);
            }
        }

        /// <summary>
        /// Verifies that --output-locale only affects winget strings while --locale continues to
        /// resolve the manifest localization.
        /// </summary>
        [Test]
        public void OutputLocaleArgumentIsIndependentOfLocaleArgument()
        {
            // The zh-CN localization of AppInstallerTest.MultipleLocale provides the package name and publisher.
            var german = TestCommon.RunAICLICommand("show", "AppInstallerTest.MultipleLocale --locale zh-CN --output-locale de-DE");
            Assert.That(german.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            // Manifest strings come from the --locale localization.
            Assert.That(german.StdOut, Does.Contain("localePackageName"));
            Assert.That(german.StdOut, Does.Contain("localePublisher"));

            // WinGet's own labels come from --output-locale.
            Assert.That(german.StdOut, Does.Contain("Herausgeber:"));
            Assert.That(german.StdOut, Does.Not.Contain("Publisher:"));

            var english = TestCommon.RunAICLICommand("show", "AppInstallerTest.MultipleLocale --locale zh-CN --output-locale en-US");
            Assert.That(english.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            // Manifest strings are unchanged by the output locale.
            Assert.That(english.StdOut, Does.Contain("localePackageName"));
            Assert.That(english.StdOut, Does.Contain("localePublisher"));

            // WinGet's own labels follow --output-locale.
            Assert.That(english.StdOut, Does.Contain("Publisher:"));
            Assert.That(english.StdOut, Does.Not.Contain("Herausgeber:"));
        }

        /// <summary>
        /// Verifies that an unsupported locale is rejected and the supported values are listed.
        /// </summary>
        [Test]
        public void OutputLocaleArgumentRejectsUnsupportedLocale()
        {
            // Well formed BCP47 tag that winget does not ship resources for.
            var result = TestCommon.RunAICLICommand(string.Empty, "--info --output-locale en-GB");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_INVALID_CL_ARGUMENTS));
            Assert.That(result.StdOut, Does.Contain("output-locale"));
            Assert.That(result.StdOut, Does.Contain("de-DE"));

            var invalid = TestCommon.RunAICLICommand(string.Empty, "--info --output-locale not-a-locale");
            Assert.That(invalid.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_INVALID_CL_ARGUMENTS));
        }
    }
}
