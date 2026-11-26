// -----------------------------------------------------------------------------
// <copyright file="SetUpFixture.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using System.Security;
    using System.Text;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Win32;
    using NUnit.Framework;

    /// <summary>
    /// Set up fixture.
    /// </summary>
    [SetUpFixture]
    public class SetUpFixture
    {
        private static bool shouldDisableDevModeOnExit = true;
        private static bool shouldRevertDefaultFileTypeRiskOnExit = true;
        private static bool shouldDoAnyTeardown = true;
        private static string defaultFileTypes = string.Empty;
        private static Encoding defaultOutputEncoding;

        /// <summary>
        /// Set up.
        /// </summary>
        [OneTimeSetUp]
        public void Setup()
        {
            UseUtf8ConsoleEncoding();

            var testParams = TestSetup.Parameters;

            if (testParams.IsDefault)
            {
                // If no parameters are provided, use defaults that work locally.
                // This allows the user to assume responsibility for setup.
                shouldDoAnyTeardown = false;
            }
            else
            {
                shouldDisableDevModeOnExit = this.EnableDevMode(true);

                shouldRevertDefaultFileTypeRiskOnExit = this.DecreaseFileTypeRisk(".exe;.msi", false);

                if (testParams.PackagedContext)
                {
                    if (testParams.LooseFileRegistration)
                    {
                        Assert.That(TestCommon.InstallMsixRegister(testParams.AICLIPackagePath), Is.True, $"InstallMsixRegister : {testParams.AICLIPackagePath}");
                    }
                    else
                    {
                        Assert.That(TestCommon.InstallMsix(testParams.AICLIPackagePath), Is.True, $"InstallMsix : {testParams.AICLIPackagePath}");
                    }
                }
            }

            if (!testParams.SkipTestSource)
            {
                TestIndex.GenerateE2ESource();
            }

            WinGetSettingsHelper.ForcedExperimentalFeatures = testParams.ForcedExperimentalFeatures;
            WinGetSettingsHelper.InitializeWingetSettings();
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [OneTimeTearDown]
        public void TearDown()
        {
            RestoreConsoleEncoding();

            if (shouldDoAnyTeardown)
            {
                if (shouldDisableDevModeOnExit)
                {
                    this.EnableDevMode(false);
                }

                if (shouldRevertDefaultFileTypeRiskOnExit)
                {
                    this.DecreaseFileTypeRisk(defaultFileTypes, true);
                }

                TestCommon.PublishE2ETestLogs();

                if (TestSetup.Parameters.PackagedContext)
                {
                    TestCommon.RemoveMsix(Constants.AICLIPackageName);
                }
            }
        }

        // Pins the console output code page to UTF-8 while the tests run, so that winget sees a
        // UTF-8 consumer and keeps writing UTF-8 to the pipes that these tests capture. Captured
        // output is decoded with the same encoding, which keeps the tests independent of the code
        // page that the machine uses.
        private static void UseUtf8ConsoleEncoding()
        {
            try
            {
                defaultOutputEncoding = Console.OutputEncoding;
                Console.OutputEncoding = Encoding.UTF8;
            }
            catch (IOException)
            {
                // There is no console to configure, so there is no code page for children to inherit.
            }
            catch (SecurityException)
            {
                // The console code page cannot be changed; the machine default is used instead.
            }
        }

        // Restores the console output code page that the test host started with.
        private static void RestoreConsoleEncoding()
        {
            if (defaultOutputEncoding == null)
            {
                return;
            }

            try
            {
                Console.OutputEncoding = defaultOutputEncoding;
            }
            catch (IOException)
            {
                // There is no console to configure.
            }
            catch (SecurityException)
            {
                // The console code page cannot be changed.
            }
        }

        // Returns whether there's a change to the dev mode state after execution
        private bool EnableDevMode(bool enable)
        {
            var appModelUnlockKey = Registry.LocalMachine.CreateSubKey(@"SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock");

            if (enable)
            {
                var value = appModelUnlockKey.GetValue("AllowDevelopmentWithoutDevLicense");
                if (value == null || (int)value == 0)
                {
                    appModelUnlockKey.SetValue("AllowDevelopmentWithoutDevLicense", 1, RegistryValueKind.DWord);
                    return true;
                }
            }
            else
            {
                var value = appModelUnlockKey.GetValue("AllowDevelopmentWithoutDevLicense");
                if (value != null && ((int)value) != 0)
                {
                    appModelUnlockKey.SetValue("AllowDevelopmentWithoutDevLicense", 0, RegistryValueKind.DWord);
                    return true;
                }
            }

            return false;
        }

        private bool DecreaseFileTypeRisk(string fileTypes, bool revert)
        {
            var defaultFileTypeRiskKey = Registry.CurrentUser.CreateSubKey(@"SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Associations");
            string value = (string)defaultFileTypeRiskKey.GetValue("DefaultFileTypeRisk");

            if (revert)
            {
                defaultFileTypeRiskKey.SetValue("LowRiskFileTypes", fileTypes);
                return false;
            }
            else
            {
                if (string.IsNullOrEmpty(value))
                {
                    defaultFileTypes = string.Empty;
                    defaultFileTypeRiskKey.SetValue("LowRiskFileTypes", fileTypes);
                }
                else
                {
                    defaultFileTypes = value;
                    defaultFileTypeRiskKey.SetValue("LowRiskFileTypes", string.Concat(value, fileTypes));
                }

                return true;
            }
        }
    }
}
