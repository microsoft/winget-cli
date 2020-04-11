// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using Microsoft.Win32;
    using NUnit.Framework;
    using System;

    [SetUpFixture]
    public class SetUpFixture
    {
        private static bool ShouldDisableDevModeOnExit = true;

        [OneTimeSetUp]
        public void Setup()
        {
            TestCommon.IsPackagedContext = TestContext.Parameters.Exists(Constants.PackagedContextParameter) &&
                TestContext.Parameters.Get(Constants.PackagedContextParameter).Equals("true", StringComparison.OrdinalIgnoreCase);

            TestCommon.VerboseLogging = TestContext.Parameters.Exists(Constants.VerboseLoggingParameter) &&
                TestContext.Parameters.Get(Constants.VerboseLoggingParameter).Equals("true", StringComparison.OrdinalIgnoreCase);

            if (TestContext.Parameters.Exists(Constants.AICLIPathParameter))
            {
                TestCommon.AICLIPath = TestContext.Parameters.Get(Constants.AICLIPathParameter);
            }
            else
            {
                if (TestCommon.IsPackagedContext)
                {
                    TestCommon.AICLIPath = "AppInst.exe";
                }
                else
                {
                    TestCommon.AICLIPath = TestCommon.GetTestFile("AppInstallerCli.exe");
                }
            }

            ShouldDisableDevModeOnExit = EnableDevMode(true);

            Assert.True(TestCommon.RunCommand("certutil.exe", "-addstore -f \"TRUSTEDPEOPLE\" " + TestCommon.GetTestDataFile(Constants.AppInstallerTestCert)));
            Assert.True(TestCommon.RunCommand("certutil.exe", "-addstore -f \"TRUSTEDPEOPLE\" " + TestCommon.GetTestDataFile(Constants.IndexPackageCert)));

            Assert.True(TestCommon.InstallMsix(TestCommon.GetTestDataFile(Constants.FrameworkPackageFile)));

            if (TestCommon.IsPackagedContext)
            {
                Assert.True(TestCommon.InstallMsix(TestCommon.GetTestFile(Constants.AICLIPackageFile)));
            }
        }

        [OneTimeTearDown]
        public void TearDown()
        {
            if (ShouldDisableDevModeOnExit)
            {
                EnableDevMode(false);
            }

            TestCommon.RunCommand("certutil.exe", $"-delstore \"TRUSTEDPEOPLE\" {Constants.AppInstallerTestCertThumbprint}");
            TestCommon.RunCommand("certutil.exe", $"-delstore \"TRUSTEDPEOPLE\" {Constants.IndexPackageCertThumbprint}");

            if (TestCommon.IsPackagedContext)
            {
                TestCommon.RemoveMsix(Constants.AICLIPackagePFN);
            }

            TestCommon.RemoveMsix(Constants.FrameworkPackagePFN);
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
                if (value != null && ((UInt32)value) != 0)
                {
                    appModelUnlockKey.SetValue("AllowDevelopmentWithoutDevLicense", 0, RegistryValueKind.DWord);
                    return true;
                }
            }

            return false;
        }
    }
}
