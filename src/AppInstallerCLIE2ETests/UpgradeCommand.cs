// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using Microsoft.Win32;
    using NUnit.Framework;
    using System.IO;

    public class UpgradeCommand : BaseCommand
    {
        private const string UninstallSubKey = @"Software\Microsoft\Windows\CurrentVersion\Uninstall";
        private const string WinGetPackageIdentifier = "WinGetPackageIdentifier";
        private const string WinGetSourceIdentifier = "WinGetSourceIdentifier";

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

            var result = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestPortableExe -v 1.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            // Modify packageId and sourceId to cause mismatch.
            ModifyPortableARPEntryValue(productCode, WinGetPackageIdentifier, "testPackageId");
            ModifyPortableARPEntryValue(productCode, WinGetSourceIdentifier, "testPackageId");

            var result2 = TestCommon.RunAICLICommand("upgrade", "AppInstallerTest.TestPortableExe -v 2.0.0.0");
            DeleteTestPortableARPEntry(productCode);
            Assert.AreNotEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Portable package from a different source already exists."));
        }

        [Test]
        public void UpgradePortableForcedOverride()
        {
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestPortableExe -v 1.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            // Modify packageId and sourceId to cause mismatch.
            ModifyPortableARPEntryValue(productCode, WinGetPackageIdentifier, "testPackageId");
            ModifyPortableARPEntryValue(productCode, WinGetSourceIdentifier, "testPackageId");

            var result2 = TestCommon.RunAICLICommand("upgrade", "AppInstallerTest.TestPortableExe -v 2.0.0.0 --force");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        private void ModifyPortableARPEntryValue(string productCode, string name, string value)
        {
            using (RegistryKey uninstallRegistryKey = Registry.CurrentUser.OpenSubKey(UninstallSubKey, true))
            {
                RegistryKey entry = uninstallRegistryKey.OpenSubKey(productCode, true);
                entry.SetValue(name, value);
            }
        }

        private void DeleteTestPortableARPEntry(string productCode)
        {
            using (RegistryKey uninstallRegistryKey = Registry.CurrentUser.OpenSubKey(UninstallSubKey, true))
            {
                uninstallRegistryKey.DeleteSubKey(productCode);
            }
        }
    }
}
