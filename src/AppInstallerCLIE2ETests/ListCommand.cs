// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class ListCommand : BaseCommand
    {
        [Test]
        public void ListSelf()
        {
            var result = TestCommon.RunAICLICommand("list", Constants.AICLIPackageFamilyName);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains(Constants.AICLIPackageFamilyName));
        }

        [Test]
        public void ListAfterInstall()
        {
            System.Guid guid = System.Guid.NewGuid();
            string productCode = guid.ToString();
            var installDir = TestCommon.GetRandomTestDir();

            // DisplayName must be set to avoid conflicts with other packages that use the same exe installer.
            string displayName = "TestExeInstaller";
            string localAppDataPath = System.Environment.GetEnvironmentVariable(Constants.LocalAppData);
            string logFilePath = System.IO.Path.Combine(localAppDataPath, Constants.E2ETestLogsPathPackaged);
            logFilePath = System.IO.Path.Combine(logFilePath, "ListAfterInstall-" + System.IO.Path.GetRandomFileName() + ".log");

            var result = TestCommon.RunAICLICommand("list", productCode);
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);

            result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --override \"/InstallDir {installDir} /ProductID {productCode} /LogFile {logFilePath} /DisplayName {displayName}\"");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            result = TestCommon.RunAICLICommand("list", productCode);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExeInstaller"));
            Assert.True(result.StdOut.Contains("1.0.0.0"));
            Assert.True(result.StdOut.Contains("2.0.0.0"));
        }

        [Test]
        public void ListWithArpVersionMapping()
        {
            // No mapping performed
            ArpVersionMappingTest("AppInstallerTest.TestArpVersionSameVersion", "TestArpVersionSameVersion", "0.5", "0.5", "< 1.0");

            // Partial mapping performed(i.e. only if version falls within arp version range)
            ArpVersionMappingTest("AppInstallerTest.TestArpVersionOppositeOrder", "TestArpVersionOppositeOrder", "10.1", "1.0", "10.1");
            ArpVersionMappingTest("AppInstallerTest.TestArpVersionOppositeOrder", "TestArpVersionOppositeOrder", "9.9", "9.9", "> 2.0");

            // Full mapping performed
            ArpVersionMappingTest("AppInstallerTest.TestArpVersionSameOrder", "TestArpVersionSameOrder", "7.0", "< 1.0", "7.0");
            ArpVersionMappingTest("AppInstallerTest.TestArpVersionSameOrder", "TestArpVersionSameOrder", "10.1", "1.0", "10.1");
            ArpVersionMappingTest("AppInstallerTest.TestArpVersionSameOrder", "TestArpVersionSameOrder", "10.7", "< 2.0", "10.7");
            ArpVersionMappingTest("AppInstallerTest.TestArpVersionSameOrder", "TestArpVersionSameOrder", "11.1", "2.0", "11.1");
            ArpVersionMappingTest("AppInstallerTest.TestArpVersionSameOrder", "TestArpVersionSameOrder", "12.0", "> 2.0", "12.0");
        }

        [Test]
        public void ListWithUpgradeCode()
        {
            // Installs the MSI installer using the TestMsiInstaller package.
            // Then tries listing the TestMsiInstallerUpgradeCode package, which should
            // be correlated to it by the UpgradeCode.
            if (string.IsNullOrEmpty(TestCommon.MsiInstallerPath))
            {
                Assert.Ignore("MSI installer not available");
            }

            var installDir = TestCommon.GetRandomTestDir();
            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("install", $"TestMsiInstaller --silent -l {installDir}").ExitCode);

            var result = TestCommon.RunAICLICommand("list", "TestMsiInstallerUpgradeCode");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestMsiInstallerUpgradeCode"));
        }

        private void ArpVersionMappingTest(string packageIdentifier, string displayNameOverride, string displayVersionOverride, string expectedListVersion, string notExpectedListVersion = "")
        {
            System.Guid guid = System.Guid.NewGuid();
            string productCode = guid.ToString();
            var installDir = TestCommon.GetRandomTestDir();

            var result = TestCommon.RunAICLICommand("list", productCode);
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);

            result = TestCommon.RunAICLICommand("install", $"{packageIdentifier} --override \"/InstallDir {installDir} /ProductID {productCode} /DisplayName {displayNameOverride} /Version {displayVersionOverride}\"");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            result = TestCommon.RunAICLICommand("list", productCode);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            Assert.True(result.StdOut.Contains(packageIdentifier));
            Assert.True(result.StdOut.Contains(expectedListVersion));
            if (!string.IsNullOrEmpty(notExpectedListVersion))
            {
                Assert.False(result.StdOut.Contains(notExpectedListVersion));
            }
        }
    }
}
