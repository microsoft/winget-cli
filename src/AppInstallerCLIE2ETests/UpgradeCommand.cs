// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;

    public class UpgradeCommand : BaseCommand
    {
        [Test]
        public void UpgradePortable()
        {
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestPortableExe -v 1.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));
            
            var result2 = TestCommon.RunAICLICommand("upgrade", "AppInstallerTest.TestPortableExe");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Successfully installed"));

            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }
    }
}
