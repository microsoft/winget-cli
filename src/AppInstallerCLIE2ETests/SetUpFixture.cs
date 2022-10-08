// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using Microsoft.Win32;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using NUnit.Framework;
    using System;
    using System.IO;

    [SetUpFixture]
    public class SetUpFixture
    {
        private static bool ShouldDisableDevModeOnExit = true;
        private static bool ShouldRevertDefaultFileTypeRiskOnExit = true;
        private static string DefaultFileTypes = string.Empty;

        [OneTimeSetUp]
        public void Setup()
        {
            // Read TestParameters and set runtime variables
            TestCommon.PackagedContext = TestContext.Parameters.Exists(Constants.PackagedContextParameter) &&
                TestContext.Parameters.Get(Constants.PackagedContextParameter).Equals("true", StringComparison.OrdinalIgnoreCase);

            TestCommon.VerboseLogging = TestContext.Parameters.Exists(Constants.VerboseLoggingParameter) &&
                TestContext.Parameters.Get(Constants.VerboseLoggingParameter).Equals("true", StringComparison.OrdinalIgnoreCase);

            TestCommon.LooseFileRegistration = TestContext.Parameters.Exists(Constants.LooseFileRegistrationParameter) &&
                    TestContext.Parameters.Get(Constants.LooseFileRegistrationParameter).Equals("true", StringComparison.OrdinalIgnoreCase);

            TestCommon.InvokeCommandInDesktopPackage = TestContext.Parameters.Exists(Constants.InvokeCommandInDesktopPackageParameter) &&
                TestContext.Parameters.Get(Constants.InvokeCommandInDesktopPackageParameter).Equals("true", StringComparison.OrdinalIgnoreCase);

            if (TestContext.Parameters.Exists(Constants.AICLIPathParameter))
            {
                TestCommon.AICLIPath = TestContext.Parameters.Get(Constants.AICLIPathParameter);
            }
            else
            {
                if (TestCommon.PackagedContext)
                {
                    // For packaged context, default to AppExecutionAlias
                    TestCommon.AICLIPath = "WinGetDev.exe";
                }
                else
                {
                    TestCommon.AICLIPath = TestCommon.GetTestFile("winget.exe");
                }
            }

            if (TestContext.Parameters.Exists(Constants.AICLIPackagePathParameter))
            {
                TestCommon.AICLIPackagePath = TestContext.Parameters.Get(Constants.AICLIPackagePathParameter);
            }
            else
            {
                TestCommon.AICLIPackagePath = TestCommon.GetTestFile("AppInstallerCLIPackage.appxbundle");
            }

            if (TestCommon.LooseFileRegistration && TestCommon.InvokeCommandInDesktopPackage)
            {
                TestCommon.AICLIPath = Path.Combine(TestCommon.AICLIPackagePath, TestCommon.AICLIPath);
            }

            ShouldDisableDevModeOnExit = EnableDevMode(true);

            ShouldRevertDefaultFileTypeRiskOnExit = DecreaseFileTypeRisk(".exe;.msi", false);

            Assert.True(TestCommon.RunCommand("certutil.exe", "-addstore -f \"TRUSTEDPEOPLE\" " + TestCommon.GetTestDataFile(Constants.AppInstallerTestCert)), "Add AppInstallerTestCert");

            if (TestCommon.PackagedContext)
            {
                if (TestCommon.LooseFileRegistration)
                {
                    Assert.True(TestCommon.InstallMsixRegister(TestCommon.AICLIPackagePath), "InstallMsixRegister");
                }
                else
                {
                    Assert.True(TestCommon.InstallMsix(TestCommon.AICLIPackagePath), "InstallMsix");
                }
            }

            if (TestContext.Parameters.Exists(Constants.StaticFileRootPathParameter))
            {
                TestCommon.StaticFileRootPath = TestContext.Parameters.Get(Constants.StaticFileRootPathParameter);
            }
            else
            {
                TestCommon.StaticFileRootPath = Path.GetTempPath();
            }

            if (TestContext.Parameters.Exists(Constants.PackageCertificatePathParameter))
            {
                TestCommon.PackageCertificatePath = TestContext.Parameters.Get(Constants.PackageCertificatePathParameter);
            }

            ReadTestInstallerPaths();

            TestIndexSetup.GenerateTestDirectory();

            InitializeWingetSettings();
        }

        [OneTimeTearDown]
        public void TearDown()
        {
            if (ShouldDisableDevModeOnExit)
            {
                EnableDevMode(false);
            }

            if (ShouldRevertDefaultFileTypeRiskOnExit)
            {
                DecreaseFileTypeRisk(DefaultFileTypes, true);
            }

            TestCommon.RunCommand("certutil.exe", $"-delstore \"TRUSTEDPEOPLE\" {Constants.AppInstallerTestCertThumbprint}");

            TestCommon.PublishE2ETestLogs();

            if (TestCommon.PackagedContext)
            {
                TestCommon.RemoveMsix(Constants.AICLIPackageName);
            }
        }

        // Returns whether there's a change to the dev mode state after execution
        private bool EnableDevMode(bool enable)
        {
            var appModelUnlockKey = Registry.LocalMachine.CreateSubKey(@"SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock");

            if (enable)
            {
                var value = appModelUnlockKey.GetValue("AllowDevelopmentWithoutDevLicense");
                if (value == null || (Int32)value == 0)
                {
                    appModelUnlockKey.SetValue("AllowDevelopmentWithoutDevLicense", 1, RegistryValueKind.DWord);
                    return true;
                }
            }
            else
            {
                var value = appModelUnlockKey.GetValue("AllowDevelopmentWithoutDevLicense");
                if (value != null && ((Int32)value) != 0)
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
                    DefaultFileTypes = string.Empty;
                    defaultFileTypeRiskKey.SetValue("LowRiskFileTypes", fileTypes);
                }
                else
                {
                    DefaultFileTypes = value;
                    defaultFileTypeRiskKey.SetValue("LowRiskFileTypes", string.Concat(value, fileTypes));
                }
                return true;
            }
        }

        private void ReadTestInstallerPaths()
        {
            if (TestContext.Parameters.Exists(Constants.ExeInstallerPathParameter) 
                && File.Exists(TestContext.Parameters.Get(Constants.ExeInstallerPathParameter)))
            {
                TestCommon.ExeInstallerPath = TestContext.Parameters.Get(Constants.ExeInstallerPathParameter);
            }

            if (TestContext.Parameters.Exists(Constants.MsiInstallerPathParameter)
                && File.Exists(TestContext.Parameters.Get(Constants.MsiInstallerPathParameter)))
            {
                TestCommon.MsiInstallerPath = TestContext.Parameters.Get(Constants.MsiInstallerPathParameter);
            }

            if (TestContext.Parameters.Exists(Constants.MsixInstallerPathParameter)
                && File.Exists(TestContext.Parameters.Get(Constants.MsixInstallerPathParameter)))
            {
                TestCommon.MsixInstallerPath = TestContext.Parameters.Get(Constants.MsixInstallerPathParameter);
            }
        }

        public void InitializeWingetSettings()
        {
            string localAppDataPath = Environment.GetEnvironmentVariable(Constants.LocalAppData);

            var settingsJson = new
            {
                experimentalFeatures = new
                {
                    experimentalArg = false,
                    experimentalCmd = false,
                    dependencies = false,
                    directMSI = false,
                    openLogsArgument = false,
                },
                debugging = new
                {
                    enableSelfInitiatedMinidump = true
                },
                installBehavior = new
                {
                    portablePackageUserRoot = "",
                    portablePackageMachineRoot = "",
                }
            };

            // Run winget one time to initialize settings directory
            // when running in unpackaged context
            TestCommon.RunAICLICommand(string.Empty, "-v");

            var serializedSettingsJson = JsonConvert.SerializeObject(settingsJson, Formatting.Indented);
            File.WriteAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath), serializedSettingsJson);
        }
    }
}
