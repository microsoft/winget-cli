// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;

    public class UninstallCommand
    {
        private const string UninstallTestSourceName = @"InstallTestSource";
        private const string UninstallTestSourceUrl = @"https://localhost:5001/TestKit";

        private const string UninstallTestExeInstalledFile = @"TestExeUninstalled.txt";

        [SetUp]
        public void Setup()
        {
            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("source add", $"{UninstallTestSourceName} {UninstallTestSourceUrl}").ExitCode);

        }

        [TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source remove", UninstallTestSourceName);
            TestCommon.WaitForDeploymentFinish();
        }

        //[Test]
        public void UninstallTestExe()
        {
            // Example Uninstall Command Test
            var installDir = TestCommon.GetRandomTestDir();
            TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {installDir}");
            var result = TestCommon.RunAICLICommand("uninstall", $"AppInstallerTest.TextExeInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully uninstalled"));
            Assert.True(VerifyTestExeUninstalled(installDir));
        }

        private bool VerifyTestExeUninstalled(string uninstallDir)
        {
            if (!File.Exists(Path.Combine(uninstallDir, UninstallTestExeInstalledFile)))
            {
                return false;
            }

            return true;
        }
    }
}
