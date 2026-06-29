// -----------------------------------------------------------------------------
// <copyright file="ListCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// List command tests.
    /// </summary>
    public class ListCommand : BaseCommand
    {
        /// <summary>
        /// Test list winget.
        /// </summary>
        [Test]
        public void ListSelf()
        {
            var result = TestCommon.RunAICLICommand("list", Constants.AICLIPackageFamilyName);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains(Constants.AICLIPackageName), Is.True);
            Assert.That(result.StdOut.Contains(Constants.AICLIPackagePublisherHash), Is.True);
        }

        /// <summary>
        /// Test list after installing a package.
        /// </summary>
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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));

            result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --override \"/InstallDir {installDir} /ProductID {productCode} /LogFile {logFilePath} /DisplayName {displayName}\"");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            result = TestCommon.RunAICLICommand("list", productCode);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("AppInstallerTest.TestExeInstaller"), Is.True);
            Assert.That(result.StdOut.Contains("1.0.0.0"), Is.True);
            Assert.That(result.StdOut.Contains("2.0.0.0"), Is.True);
        }

        /// <summary>
        /// Test expected entries after list.
        /// </summary>
        [Test]
        public void ListWithArpVersionMapping()
        {
            // No mapping performed
            this.ArpVersionMappingTest("AppInstallerTest.TestArpVersionSameVersion", "TestArpVersionSameVersion", "0.5", "0.5", "< 1.0");

            // Partial mapping performed(i.e. only if version falls within arp version range)
            this.ArpVersionMappingTest("AppInstallerTest.TestArpVersionOppositeOrder", "TestArpVersionOppositeOrder", "10.1", "1.0", "10.1");
            this.ArpVersionMappingTest("AppInstallerTest.TestArpVersionOppositeOrder", "TestArpVersionOppositeOrder", "9.9", "9.9", "> 2.0");

            // Full mapping performed
            this.ArpVersionMappingTest("AppInstallerTest.TestArpVersionSameOrder", "TestArpVersionSameOrder", "7.0", "< 1.0", "7.0");
            this.ArpVersionMappingTest("AppInstallerTest.TestArpVersionSameOrder", "TestArpVersionSameOrder", "10.1", "1.0", "10.1");
            this.ArpVersionMappingTest("AppInstallerTest.TestArpVersionSameOrder", "TestArpVersionSameOrder", "10.7", "< 2.0", "10.7");
            this.ArpVersionMappingTest("AppInstallerTest.TestArpVersionSameOrder", "TestArpVersionSameOrder", "11.1", "2.0", "11.1");
            this.ArpVersionMappingTest("AppInstallerTest.TestArpVersionSameOrder", "TestArpVersionSameOrder", "12.0", "> 2.0", "12.0");
        }

        /// <summary>
        /// Test list with upgrade code.
        /// </summary>
        [Test]
        public void ListWithUpgradeCode()
        {
            // Installs the MSI installer using the TestMsiInstaller package.
            // Then tries listing the TestMsiInstallerUpgradeCode package, which should
            // be correlated to it by the UpgradeCode.
            if (string.IsNullOrEmpty(TestIndex.MsiInstaller))
            {
                Assert.Ignore("MSI installer not available");
            }

            var installDir = TestCommon.GetRandomTestDir();
            Assert.That(TestCommon.RunAICLICommand("install", $"TestMsiInstaller --silent -l {installDir}").ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            var result = TestCommon.RunAICLICommand("list", "TestMsiInstallerUpgradeCode");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("AppInstallerTest.TestMsiInstallerUpgradeCode"), Is.True);
        }

        /// <summary>
        /// Test list with exe installed with machine scope.
        /// </summary>
        [Test]
        public void ListWithScopeExeInstalledAsMachine()
        {
            System.Guid guid = System.Guid.NewGuid();
            string identifier = "AppInstallerTest.TestExeInstaller";
            string productCode = guid.ToString();
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"{identifier} --override \"/InstallDir {installDir} /ProductID {productCode} /UseHKLM\"");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            // List with user scope will not find the package
            result = TestCommon.RunAICLICommand("list", $"{productCode} --scope user");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));
            Assert.That(result.StdOut.Contains(identifier), Is.False);

            // List with machine scope will find the package
            result = TestCommon.RunAICLICommand("list", $"{productCode} --scope machine");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains(identifier), Is.True);

            TestCommon.RunCommand(Path.Combine(installDir, Constants.TestExeUninstallerFileName));
        }

        /// <summary>
        /// Test list with exe installed with user scope.
        /// </summary>
        [Test]
        public void ListWithScopeExeInstalledAsUser()
        {
            System.Guid guid = System.Guid.NewGuid();
            string identifier = "AppInstallerTest.TestExeInstaller";
            string productCode = guid.ToString();
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"{identifier} --override \"/InstallDir {installDir} /ProductID {productCode}\"");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            // List with user scope will find the package
            result = TestCommon.RunAICLICommand("list", $"{productCode} --scope user");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains(identifier), Is.True);

            // List with machine scope will not find the package
            result = TestCommon.RunAICLICommand("list", $"{productCode} --scope machine");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));
            Assert.That(result.StdOut.Contains(identifier), Is.False);

            TestCommon.RunCommand(Path.Combine(installDir, Constants.TestExeUninstallerFileName));
        }

        /// <summary>
        /// Test list with msix installed with machine scope.
        /// </summary>
        [Test]
        public void ListWithScopeMsixInstalledAsMachine()
        {
            // TODO: Provision and Deprovision api not supported in build server.
            Assert.Ignore();

            var result = TestCommon.RunAICLICommand("install", $"{Constants.MsixInstallerPackageId} --scope machine");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            // List with user scope will also find the package because msix is provisioned for all users
            result = TestCommon.RunAICLICommand("list", $"{Constants.MsixInstallerPackageId} --scope user");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains(Constants.MsixInstallerPackageId), Is.True);

            // List with machine scope will find the package
            result = TestCommon.RunAICLICommand("list", $"{Constants.MsixInstallerPackageId} --scope machine");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains(Constants.MsixInstallerPackageId), Is.True);

            TestCommon.RemoveMsix(Constants.MsixInstallerName, true);
        }

        /// <summary>
        /// Test list with msix installed with user scope.
        /// </summary>
        [Test]
        public void ListWithScopeMsixInstalledAsUser()
        {
            var result = TestCommon.RunAICLICommand("install", $"{Constants.MsixInstallerPackageId} --scope user");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            // List with user scope will find the package
            result = TestCommon.RunAICLICommand("list", $"{Constants.MsixInstallerPackageId} --scope user");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains(Constants.MsixInstallerPackageId), Is.True);

            // List with machine scope will not find the package
            result = TestCommon.RunAICLICommand("list", $"{Constants.MsixInstallerPackageId} --scope machine");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));
            Assert.That(result.StdOut.Contains(Constants.MsixInstallerPackageId), Is.False);

            TestCommon.RemoveMsix(Constants.MsixInstallerName);
        }

        /// <summary>
        /// Test package correlation with same package name but different architecture is correct.
        /// </summary>
        [Test]
        public void ListWithMappingWithArchitecture()
        {
            var installDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestMappingWithArchitectureX86 -l {installDir}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            // List with AppInstallerTest.TestMappingWithArchitectureX64 (from available to installed scenario) will not find the package.
            result = TestCommon.RunAICLICommand("list", "AppInstallerTest.TestMappingWithArchitectureX64");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));
            Assert.That(result.StdOut.Contains("AppInstallerTest.TestMappingWithArchitectureX64"), Is.False);

            // List with AppInstallerTest.TestMappingWithArchitectureX86 (from available to installed scenario) will find the package.
            result = TestCommon.RunAICLICommand("list", "AppInstallerTest.TestMappingWithArchitectureX86");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("AppInstallerTest.TestMappingWithArchitectureX86"), Is.True);

            // List with product code (from installed to available scenario) will find the AppInstallerTest.TestMappingWithArchitectureX86 package.
            result = TestCommon.RunAICLICommand("list", "{0e426f01-b682-4e67-a357-52f9ecb4590d}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("AppInstallerTest.TestMappingWithArchitectureX86"), Is.True);

            // best effort clean up
            TestCommon.RunCommand(Path.Combine(installDir, Constants.TestExeUninstallerFileName));
        }

        private void ArpVersionMappingTest(string packageIdentifier, string displayNameOverride, string displayVersionOverride, string expectedListVersion, string notExpectedListVersion = "")
        {
            System.Guid guid = System.Guid.NewGuid();
            string productCode = guid.ToString();
            var installDir = TestCommon.GetRandomTestDir();

            var result = TestCommon.RunAICLICommand("list", productCode);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));

            result = TestCommon.RunAICLICommand("install", $"{packageIdentifier} --override \"/InstallDir {installDir} /ProductID {productCode} /DisplayName {displayNameOverride} /Version {displayVersionOverride}\"");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            result = TestCommon.RunAICLICommand("list", productCode);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            Assert.That(result.StdOut.Contains(packageIdentifier), Is.True);
            Assert.That(result.StdOut.Contains(expectedListVersion), Is.True);
            if (!string.IsNullOrEmpty(notExpectedListVersion))
            {
                Assert.That(result.StdOut.Contains(notExpectedListVersion), Is.False);
            }

            // Try clean up
            if (File.Exists(Path.Combine(installDir, Constants.TestExeInstalledFileName)))
            {
                TestCommon.RunCommand(Path.Combine(installDir, Constants.TestExeUninstallerFileName));
            }
        }
    }
}
