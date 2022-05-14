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

        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            ConfigureFeature("portableInstall", true);
        }

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
            
            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result2.ExitCode);
            Assert.True(result2.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        [Test]
        public void UpgradePortableARPMismatch()
        {
            string packageId = "AppInstallerTest.TestPortableExe";
            string productCode = packageId + "_" + Constants.TestSourceIdentifier;

            var installResult = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestPortableExe -v 1.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode);
            Assert.True(installResult.StdOut.Contains("Successfully installed"));

            // Modify packageId to cause mismatch.
            ModifyPortableARPEntryValue(productCode, WinGetPackageIdentifier, "testPackageId");

            var upgradeResult = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0");

            // Reset and perform uninstall cleanup
            ModifyPortableARPEntryValue(productCode, WinGetPackageIdentifier, packageId);
            TestCommon.RunAICLICommand("uninstall", $"--product-code {productCode}");

            Assert.AreNotEqual(Constants.ErrorCode.S_OK, upgradeResult.ExitCode);
            Assert.True(upgradeResult.StdOut.Contains("Portable package from a different source already exists"));
        }

        [Test]
        public void UpgradePortableForcedOverride()
        {
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var installResult = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestPortableExe -v 1.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode);
            Assert.True(installResult.StdOut.Contains("Successfully installed"));

            // Modify packageId and sourceId to cause mismatch.
            ModifyPortableARPEntryValue(productCode, WinGetPackageIdentifier, "testPackageId");
            ModifyPortableARPEntryValue(productCode, WinGetSourceIdentifier, "testSourceId");

            var upgradeResult = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 2.0.0.0 --force");
            Assert.AreEqual(Constants.ErrorCode.S_OK, upgradeResult.ExitCode);
            Assert.True(upgradeResult.StdOut.Contains("Successfully installed"));
            TestCommon.VerifyPortablePackage(Path.Combine(installDir, packageDirName), commandAlias, fileName, productCode, true);
        }

        [Test]
        public void UpgradePortableUninstallPrevious()
        {
            string installDir = Path.Combine(System.Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
            string packageId, commandAlias, fileName, packageDirName, productCode;
            packageId = "AppInstallerTest.TestPortableExe";
            packageDirName = productCode = packageId + "_" + Constants.TestSourceIdentifier;
            commandAlias = fileName = "AppInstallerTestExeInstaller.exe";

            var result = TestCommon.RunAICLICommand("install", $"{packageId} -v 1.0.0.0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Successfully installed"));

            var result2 = TestCommon.RunAICLICommand("upgrade", $"{packageId} -v 3.0.0.0");
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
    }
}
