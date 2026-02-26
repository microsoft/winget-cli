// -----------------------------------------------------------------------------
// <copyright file="SetUpFixture.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
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

        /// <summary>
        /// Set up.
        /// </summary>
        [OneTimeSetUp]
        public void Setup()
        {
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
                        Assert.True(TestCommon.InstallMsixRegister(testParams.AICLIPackagePath), $"InstallMsixRegister : {testParams.AICLIPackagePath}");
                    }
                    else
                    {
                        Assert.True(TestCommon.InstallMsix(testParams.AICLIPackagePath), $"InstallMsix : {testParams.AICLIPackagePath}");
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
