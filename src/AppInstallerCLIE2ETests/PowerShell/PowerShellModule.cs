// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.PowerShell
{
    using NUnit.Framework;
    using System;
    using System.Diagnostics;
    using System.Linq;
    using System.Threading;

    /// <summary>
    /// Basic E2E tests for verifying that behavior of the PowerShell module cmdlets.
    /// </summary>
    [Category("PowerShell")]
    public class PowerShellModule
    {
        [OneTimeSetUp]
        public void Setup()
        {
            TestCommon.RunAICLICommand("source add", $"-n {Constants.TestSourceName} {Constants.TestSourceUrl}");
        }

        [OneTimeTearDown]
        public void TearDown()
        {
            // TODO: This is a workaround to an issue where the server takes longer than expected to terminate when
            // running from the E2E tests. This can cause other E2E tests to fail when attempting to reset the test source.
            if (IsRunning(Constants.WinGetServerExeName))
            {
                // There should only be one WinGetServer process running at a time.
                Process serverProcess = Process.GetProcessesByName(Constants.WinGetServerExeName).First();
                serverProcess.Kill();
            }

            TestCommon.RunAICLICommand("source remove", $"{Constants.TestSourceName}");
        }

        [Test]
        public void AssertServerShutdownAfterExecution()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            TestCommon.RunPowerShellCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            
            // Wait for 15 seconds and verify that WingetServer process is no longer running.
            Thread.Sleep(15000);
            Assert.IsTrue(!IsRunning(Constants.WinGetServerExeName), $"{Constants.WinGetServerExeName} failed to terminate after creating COM object.");
        }

        [Test]
        public void GetWinGetSource()
        {
            // Running the x86 PowerShell Module requires PowerShell Core (x86). The tests currently only target PowerShell Core (x64)
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var getSourceResult = TestCommon.RunPowerShellCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            Assert.IsTrue(getSourceResult.ExitCode == 0, $"ExitCode: {getSourceResult.ExitCode} Failed with the following output: {getSourceResult.StdOut}, {getSourceResult.StdErr}");
            Assert.IsTrue(getSourceResult.StdOut.Contains($"{Constants.TestSourceName}"));
        }

        [Test]
        public void FindWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var result = TestCommon.RunPowerShellCommandWithResult(Constants.FindCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            Assert.IsTrue(result.ExitCode == 0, $"ExitCode: {result.ExitCode} Failed with the following output: {result.StdOut}; {result.StdErr}");
            Assert.IsTrue(result.StdOut.Contains("TestExeInstaller"));
        }

        [Test]
        public void GetWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

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
            if (!Environment.Is64BitProcess)
            {
                return;
            }

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
            if (!Environment.Is64BitProcess)
            {
                return;
            }

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

        private bool IsRunning(string processName)
        {
            return Process.GetProcessesByName(processName).Length > 0;
        }
    }
}