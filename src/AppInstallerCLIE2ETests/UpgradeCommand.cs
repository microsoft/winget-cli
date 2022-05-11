// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using Microsoft.Win32;
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
            
            var result2 = TestCommon.RunAICLICommand("upgrade", "AppInstallerTest.TestPortableExe -v 2.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        [Test]
        public void UpgradePortableUninstallPrevious()
        {
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_CompositeSource";
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestPortableExe -v 1.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", "AppInstallerTest.TestPortableExe -v 3.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        [Test]
        public void UpgradePortableARPMismatch()
        {
            string packageId = "AppInstallerTest.TestPortableExe";
            string productCode = packageId + "_" + Constants.TestSourceIdentifier;
            RegistryKey testEntry = CreateTestPortableARPEntry(productCode);

            var result = TestCommon.RunAICLICommand("upgrade", "AppInstallerTest.TestPortableExe -v 2.0.0.0");
            testEntry.DeleteSubKey(productCode);
            Assert.AreNotEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Portable package from a different source already exists."));
        }

        [Test]
        public void UpgradePortableForcedOverride()
        {
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            CreateTestPortableARPEntry(productCode);
            var result2 = TestCommon.RunAICLICommand("upgrade", "AppInstallerTest.TestPortableExe -v 2.0.0.0 --force");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        /// <summary>
        /// Creates a portable ARP entry for testing purposes.
        /// </summary>
        /// <param name="productCode">ProductCode to be used as the entry name.</param>
        /// <returns>The uninstall registry key containing the created entry.</returns>
        private RegistryKey CreateTestPortableARPEntry(string productCode)
        {
            string subKey = @$"Software\Microsoft\Windows\CurrentVersion\Uninstall";
            using (RegistryKey uninstallRegistryKey = Registry.CurrentUser.OpenSubKey(subKey, true))
            {
                RegistryKey duplicateEntry = uninstallRegistryKey.CreateSubKey(productCode, true);
                duplicateEntry.SetValue("WinGetPackageIdentifier", "testPackageId");
                duplicateEntry.SetValue("WinGetSourceIdentifier", "testSourceId");
                return uninstallRegistryKey;
            }
        }
    }
}
