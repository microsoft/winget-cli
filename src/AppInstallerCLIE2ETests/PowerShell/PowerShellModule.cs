// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    /// <summary>
    /// Basic E2E tests for verifying that behavior of the PowerShell module cmdlets.
    /// </summary>
    public class APowerShellModule
    {
        [OneTimeSetUp]
        public void Setup()
        {
            // Add-WinGetPackage is a function and not a cmdlet that uses COM. Add source to WinGetDev directly to ensure test source exists.
            TestCommon.RunAICLICommand("source add", $"-n {Constants.TestSourceName} {Constants.TestSourceUrl}");
        }

        [OneTimeTearDown]
        public void TearDown()
        {
            // Remove-WinGetPackage is a function and not a cmdlet that uses COM. Remove source from WinGetDev directory to ensure clean state.
            TestCommon.RunAICLICommand("source remove", $"-n {Constants.TestSourceName}");
        }

        [Test]
        public void GetWinGetSource()
        {
            var getSourceResult = TestCommon.RunPowerShellCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            Assert.IsTrue(getSourceResult.ExitCode == 0, $"ExitCode: {getSourceResult.ExitCode} Failed with the following output: {getSourceResult.StdOut}, {getSourceResult.StdErr}");
            Assert.IsTrue(getSourceResult.StdOut.Contains($"{Constants.TestSourceName}"));
        }

        [Test]
        public void FindWinGetPackage()
        {
            var result = TestCommon.RunPowerShellCommandWithResult(Constants.FindCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            Assert.IsTrue(result.ExitCode == 0, $"ExitCode: {result.ExitCode} Failed with the following output: {result.StdOut}; {result.StdErr}");
            Assert.IsTrue(result.StdOut.Contains("TestExeInstaller"));
        }

        [Test]
        public void GetWinGetPackage()
        {
            var installResult = TestCommon.RunPowerShellCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.MsiInstallerPackageId}");
            var getResult = TestCommon.RunPowerShellCommandWithResult(Constants.GetCmdlet, $"-Id {Constants.MsiInstallerPackageId}");
            var uninstallResult = TestCommon.RunPowerShellCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.MsiInstallerPackageId}");

            Assert.IsTrue(installResult.ExitCode == 0, $"ExitCode: {installResult.ExitCode}; Failed with the following output: {installResult.StdOut}; {installResult.StdErr}");
            Assert.IsTrue(getResult.ExitCode == 0, $"Failed with the following output: {getResult.StdOut}");
            Assert.IsTrue(uninstallResult.ExitCode == 0, $"Failed with the following output: {uninstallResult.StdOut}");

            Assert.IsTrue(!string.IsNullOrEmpty(installResult.StdOut));
            Assert.IsTrue(getResult.StdOut.Contains("TestMsiInstaller"));
            Assert.IsTrue(!string.IsNullOrEmpty(uninstallResult.StdOut));
        }

        [Test]
        public void InstallWinGetPackage()
        {
            var installResult = TestCommon.RunPowerShellCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            var uninstallResult = TestCommon.RunPowerShellCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");

            Assert.IsTrue(installResult.ExitCode == 0, $"ExitCode: {installResult.ExitCode}; Failed with the following output: {installResult.StdOut}; {installResult.StdErr}");
            Assert.IsTrue(uninstallResult.ExitCode == 0, $"Failed with the following output: {uninstallResult.StdOut}");

            Assert.IsTrue(!string.IsNullOrEmpty(installResult.StdOut));
            Assert.IsTrue(!string.IsNullOrEmpty(uninstallResult.StdOut));
        }

        [Test]
        public void UpdateWinGetPackage()
        {
            var installResult = TestCommon.RunPowerShellCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.ExeInstallerPackageId} -Version 1.0.0.0");
            var updateResult = TestCommon.RunPowerShellCommandWithResult(Constants.UpdateCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            var getResult = TestCommon.RunPowerShellCommandWithResult(Constants.GetCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            var uninstallResult = TestCommon.RunPowerShellCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");

            Assert.IsTrue(installResult.ExitCode == 0, $"Failed with the following output: {installResult.StdOut}");
            Assert.IsTrue(updateResult.ExitCode == 0, $"Failed with the following output: {updateResult.StdOut}");
            Assert.IsTrue(getResult.ExitCode == 0, $"Failed with the following output: {getResult.StdOut}");
            Assert.IsTrue(uninstallResult.ExitCode == 0, $"Failed with the following output: {uninstallResult.StdOut}");

            Assert.IsTrue(!string.IsNullOrEmpty(installResult.StdOut));
            Assert.IsTrue(!string.IsNullOrEmpty(updateResult.StdOut));
            Assert.IsTrue(getResult.StdOut.Contains("2.0.0.0"));
            Assert.IsTrue(!string.IsNullOrEmpty(uninstallResult.StdOut));
        }
    }
}