// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;

    public class UninstallCommand : BaseCommand
    {
        private const string UninstallTestExeInstalledFile = @"TestExeUninstalled.txt";

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
            return File.Exists(Path.Combine(uninstallDir, UninstallTestExeInstalledFile));
        }
    }
}
