// -----------------------------------------------------------------------------
// <copyright file="WinGetClientModule.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.PowerShell
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Linq;
    using System.Management.Automation;
    using NUnit.Framework;

    /// <summary>
    /// Basic E2E smoke tests for verifying the behavior of the PowerShell Microsoft.WinGet.Client module cmdlets.
    /// Running the x86 PowerShell Module requires PowerShell Core (x86). These tests currently only target PowerShell Core (x64)
    /// in the CI/CD pipeline.
    /// </summary>
    [Category("PowerShell")]
    public class WinGetClientModule
    {
        // TODO: Consider using Pester framework for conducting more extensive PowerShell module tests or move to Powershell Host.

        /// <summary>
        /// Set setup.
        /// </summary>
        [OneTimeSetUp]
        public void Setup()
        {
            TestCommon.RunAICLICommand("source add", $"-n {Constants.TestSourceName} {Constants.TestSourceUrl}");
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [OneTimeTearDown]
        public void TearDown()
        {
            // TODO: This is a workaround to an issue where the server takes longer than expected to terminate when
            // running from the E2E tests. This can cause other E2E tests to fail when attempting to reset the test source.
            if (this.IsRunning(Constants.WindowsPackageManagerServer))
            {
                // There should only be one WinGetServer process running at a time.
                Process serverProcess = Process.GetProcessesByName(Constants.WindowsPackageManagerServer).First();
                serverProcess.Kill();
            }

            TestCommon.RunAICLICommand("source remove", $"{Constants.TestSourceName}");
            WinGetSettingsHelper.InitializeWingetSettings();
        }

        /// <summary>
        /// Assert server shutdown.
        /// </summary>
        [Test]
        public void AssertServerShutdownAfterExecution()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var result = TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode, $"ExitCode: {result.ExitCode} Failed with the following output: {result.StdOut}, {result.StdErr}");

            Assert.IsTrue(this.IsRunning(Constants.WindowsPackageManagerServer), $"{Constants.WindowsPackageManagerServer} is not running.");
            Process serverProcess = Process.GetProcessesByName(Constants.WindowsPackageManagerServer).First();

            // Wait a maximum of 30 seconds for the server process to exit.
            bool serverProcessExit = serverProcess.WaitForExit(30000);
            Assert.IsTrue(serverProcessExit, $"{Constants.WindowsPackageManagerServer} failed to terminate after creating COM object.");
        }

        /// <summary>
        /// Test Get-WinGetSource.
        /// </summary>
        [Test]
        public void GetWinGetSource()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var getSourceResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, getSourceResult.ExitCode, $"ExitCode: {getSourceResult.ExitCode} Failed with the following output: {getSourceResult.StdOut}, {getSourceResult.StdErr}");
            Assert.IsTrue(getSourceResult.StdOut.Contains($"{Constants.TestSourceName}"));
        }

        /// <summary>
        /// Find-WinGetPackage.
        /// </summary>
        [Test]
        public void FindWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var result = TestCommon.RunPowerShellCoreCommandWithResult(Constants.FindCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode, $"ExitCode: {result.ExitCode} Failed with the following output: {result.StdOut}; {result.StdErr}");
            Assert.IsTrue(result.StdOut.Contains("TestExeInstaller"));
        }

        /// <summary>
        /// Tests Get-WinGetPackage.
        /// </summary>
        [Test]
        public void GetWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var installResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.MsiInstallerPackageId}");
            var getResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetCmdlet, $"-Id {Constants.MsiInstallerPackageId}");
            var uninstallResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.MsiInstallerPackageId}");

            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode, $"ExitCode: {installResult.ExitCode}; Failed with the following output: {installResult.StdOut}; {installResult.StdErr}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, getResult.ExitCode, $"Failed with the following output: {getResult.StdOut}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, uninstallResult.ExitCode, $"Failed with the following output: {uninstallResult.StdOut}");

            Assert.IsTrue(!string.IsNullOrEmpty(installResult.StdOut));
            Assert.IsTrue(getResult.StdOut.Contains("TestMsiInstaller"));
            Assert.IsTrue(!string.IsNullOrEmpty(uninstallResult.StdOut));
        }

        /// <summary>
        /// Tests Install-WinGetPackage.
        /// </summary>
        [Test]
        public void InstallWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var installResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            var uninstallResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");

            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode, $"ExitCode: {installResult.ExitCode}; Failed with the following output: {installResult.StdOut}; {installResult.StdErr}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, uninstallResult.ExitCode, $"Failed with the following output: {uninstallResult.StdOut}");

            Assert.IsTrue(!string.IsNullOrEmpty(installResult.StdOut));
            Assert.IsTrue(!string.IsNullOrEmpty(uninstallResult.StdOut));
        }

        /// <summary>
        /// Tests Update-WinGetPackage.
        /// </summary>
        [Test]
        public void UpdateWinGetPackage()
        {
            if (!Environment.Is64BitProcess)
            {
                return;
            }

            var installResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.ExeInstallerPackageId} -Version 1.0.0.0");
            var updateResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.UpdateCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            var getResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            var uninstallResult = TestCommon.RunPowerShellCoreCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");

            Assert.AreEqual(Constants.ErrorCode.S_OK, installResult.ExitCode, $"Failed with the following output: {installResult.StdOut}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, updateResult.ExitCode, $"Failed with the following output: {updateResult.StdOut}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, getResult.ExitCode, $"Failed with the following output: {getResult.StdOut}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, uninstallResult.ExitCode, $"Failed with the following output: {uninstallResult.StdOut}");

            Assert.IsTrue(!string.IsNullOrEmpty(installResult.StdOut));
            Assert.IsTrue(!string.IsNullOrEmpty(updateResult.StdOut));
            Assert.IsTrue(getResult.StdOut.Contains("2.0.0.0"));
            Assert.IsTrue(!string.IsNullOrEmpty(uninstallResult.StdOut));
        }

        /// <summary>
        /// There is a known issue where the server takes an abnormally long time to terminate after the E2E test pwsh processes finish execution.
        /// This test verifies that the server does indeed terminate within 5 minutes after running all of the cmdlets.
        /// Commented out to reduce the overall duration of the build pipeline.
        /// </summary>
        [Test]
        [Ignore("Ignoring")]
        public void VerifyServerTermination()
        {
            TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetSourceCmdlet, $"-Name {Constants.TestSourceName}");
            TestCommon.RunPowerShellCoreCommandWithResult(Constants.FindCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            TestCommon.RunPowerShellCoreCommandWithResult(Constants.InstallCmdlet, $"-Id {Constants.ExeInstallerPackageId} -Version 1.0.0.0");
            TestCommon.RunPowerShellCoreCommandWithResult(Constants.UpdateCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            TestCommon.RunPowerShellCoreCommandWithResult(Constants.GetCmdlet, $"-Id {Constants.ExeInstallerPackageId}");
            TestCommon.RunPowerShellCoreCommandWithResult(Constants.UninstallCmdlet, $"-Id {Constants.ExeInstallerPackageId}");

            Assert.IsTrue(this.IsRunning(Constants.WindowsPackageManagerServer), $"{Constants.WindowsPackageManagerServer} is not running.");
            Process serverProcess = Process.GetProcessesByName(Constants.WindowsPackageManagerServer).First();

            // Wait a maximum of 5 minutes for the server process to exit.
            bool serverProcessExit = serverProcess.WaitForExit(300000);
            Assert.IsTrue(serverProcessExit, $"{Constants.WindowsPackageManagerServer} failed to terminate after creating COM object.");
        }

        /// <summary>
        /// Test Get-WinGetUserSettings.
        /// </summary>
        [Test]
        public void GetWinGetUserSettings()
        {
            var ogSettings = @"{
  ""visual"": {
    ""progressBar"": ""rainbow""
  },
  ""experimentalFeatures"": {
    ""experimentalArg"": false,
    ""experimentalCmd"": true
  }
}";

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Get-WinGetUserSettings")
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<Hashtable>(result[0].BaseObject);
        }

        /// <summary>
        /// Test Get-WinGetUserSettings when the local settings file is not a json.
        /// </summary>
        [Test]
        public void GetWinGetUserSettings_BadJsonFile()
        {
            WinGetSettingsHelper.SetWingetSettings("Hi, im not a json. Thank you, Test.");

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();

            var cmdletException = Assert.Throws<CmdletInvocationException>(
                () => powerShellHost.PowerShell
                .AddCommand("Get-WinGetUserSettings")
                .Invoke());

            // If we reference Microsoft.WinGet.Client to this project PowerShell host fails with
            //   System.Management.Automation.CmdletInvocationException : Operation is not supported on this platform. (0x80131539)
            //   System.PlatformNotSupportedException : Operation is not supported on this platform. (0x80131539)
            // trying to load the runspace. This is most probably because the same dll is already loaded.
            // Check the type the long way.
            dynamic exception = cmdletException.InnerException;
            Assert.AreEqual(exception.GetType().ToString(), "Microsoft.WinGet.Client.Exceptions.UserSettingsReadException");
        }

        /// <summary>
        /// Test Test-WinGetUserSettings. Settings are equal.
        /// </summary>
        [Test]
        public void TestWinGetUserSettings_Equal()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Test-WinGetUserSettings")
                .AddParameter("UserSettings", ogSettings)
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<bool>(result[0].BaseObject);
            Assert.IsTrue((bool)result[0].BaseObject);
        }

        /// <summary>
        /// Test Test-WinGetUserSettings. Settings are equal. Ignore schema.
        /// </summary>
        [Test]
        public void TestWinGetUserSettings_Equal_Schema()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "$schema",
                    "https://aka.ms/winget-settings.schema.json"
                },
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Test-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<bool>(result[0].BaseObject);
            Assert.IsTrue((bool)result[0].BaseObject);
        }

        /// <summary>
        /// Test Test-WinGetUserSettings. Settings are not equal.
        /// </summary>
        [Test]
        public void TestWinGetUserSettings_NotEqual()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "rainbow" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Test-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<bool>(result[0].BaseObject);
            Assert.IsFalse((bool)result[0].BaseObject);
        }

        /// <summary>
        /// Test Test-WinGetUserSettings. Local settings has more properties.
        /// </summary>
        [Test]
        public void TestWinGetUserSettings_MoreSettingsLocal()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Test-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<bool>(result[0].BaseObject);
            Assert.IsFalse((bool)result[0].BaseObject);
        }

        /// <summary>
        /// Test Test-WinGetUserSettings. Input has more properties.
        /// </summary>
        [Test]
        public void TestWinGetUserSettings_MoreSettingsInput()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell.AddCommand("Test-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<bool>(result[0].BaseObject);
            Assert.IsFalse((bool)result[0].BaseObject);
        }

        /// <summary>
        /// Test Test-WinGetUserSettings. IgnoreNotSet.
        /// They are equal.
        /// </summary>
        [Test]
        public void TestWinGetUserSettings_Equal_IgnoreNotSet()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Test-WinGetUserSettings")
                .AddParameter("UserSettings", ogSettings)
                .AddParameter("IgnoreNotSet")
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<bool>(result[0].BaseObject);
            Assert.IsTrue((bool)result[0].BaseObject);
        }

        /// <summary>
        /// Test Test-WinGetUserSettings IgnoreNotSet.
        /// Ignore comparing properties that are not set in the input.
        /// </summary>
        [Test]
        public void TestWinGetUserSettings_MoreSettingsLocal_IgnoreNotSet()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Test-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .AddParameter("IgnoreNotSet")
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<bool>(result[0].BaseObject);
            Assert.IsTrue((bool)result[0].BaseObject);
        }

        /// <summary>
        /// Test Test-WinGetUserSettings IgnoreNotSet.
        /// Local settings doesnt have some properties.
        /// </summary>
        [Test]
        public void TestWinGetUserSettings_MoreSettingsInput_IgnoreNotSet()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Test-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .AddParameter("IgnoreNotSet")
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<bool>(result[0].BaseObject);
            Assert.IsFalse((bool)result[0].BaseObject);
        }

        /// <summary>
        /// Test Test-WinGetUserSettings IgnoreNotSet.
        /// DeepEquals fails, but we should still fail at experimentalArg.
        /// </summary>
        [Test]
        public void TestWinGetUserSettings_DifferentValue_IgnoreNotSet()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", true },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Test-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .AddParameter("IgnoreNotSet")
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<bool>(result[0].BaseObject);
            Assert.IsFalse((bool)result[0].BaseObject);
        }

        /// <summary>
        /// Test Test-WinGetUserSettings IgnoreNotSet.
        /// DeepEquals fails, but we should still fail at comparing the array.
        /// </summary>
        [Test]
        public void TestWinGetUserSettings_ArrayDifferent_IgnoreNotSet()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "installBehavior",
                    new Hashtable()
                    {
                        {
                            "preferences",
                            new Hashtable()
                            {
                                { "architectures", new string[] { "x64", "x86" } },
                            }
                        },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            var inputSettings = new Hashtable()
            {
                {
                    "installBehavior",
                    new Hashtable()
                    {
                        {
                            "preferences",
                            new Hashtable()
                            {
                                { "architectures", new string[] { "x64", "arm64" } },
                            }
                        },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Test-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .AddParameter("IgnoreNotSet")
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<bool>(result[0].BaseObject);
            Assert.IsFalse((bool)result[0].BaseObject);
        }

        /// <summary>
        /// Test Test-WinGetUserSettings IgnoreNotSet.
        /// DeepEquals fails, but we should still fail at experimentalArg because is an int.
        /// </summary>
        [Test]
        public void TestWinGetUserSettings_DifferentValueType_IgnoreNotSet()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", 4 },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Test-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .AddParameter("IgnoreNotSet")
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<bool>(result[0].BaseObject);
            Assert.IsFalse((bool)result[0].BaseObject);
        }

        /// <summary>
        /// Test Test-WinGetUserSettings.
        /// Settings file is not a json.
        /// </summary>
        [Test]
        public void TestWinGetUserSettings_BadJsonFile()
        {
            WinGetSettingsHelper.SetWingetSettings("Hi, im not a json. Thank you, Test.");

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Test-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<bool>(result[0].BaseObject);
            Assert.IsFalse((bool)result[0].BaseObject);
        }

        /// <summary>
        /// Test Set-WinGetUserSettings.
        /// </summary>
        [Test]
        public void SetWinGetUserSettings_Overwrite()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "$schema",
                    "https://aka.ms/winget-settings.schema.json"
                },
                {
                    "source",
                    new Hashtable()
                    {
                        { "autoUpdateIntervalInMinutes", 3 },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Set-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<Hashtable>(result[0].BaseObject);
            var settingsResult = result[0].BaseObject as Hashtable;

            Assert.True(settingsResult.ContainsKey("$schema"));
            Assert.False(settingsResult.ContainsKey("source"));
            Assert.True(settingsResult.ContainsKey("experimentalFeatures"));
            Assert.True(settingsResult.ContainsKey("visual"));
        }

        /// <summary>
        /// Test Set-WinGetUserSettings. Merge local settings with input.
        /// </summary>
        [Test]
        public void SetWinGetUserSettings_Merge()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "source",
                    new Hashtable()
                    {
                        { "autoUpdateIntervalInMinutes", 3 },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Set-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .AddParameter("Merge")
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<Hashtable>(result[0].BaseObject);
            var settingsResult = result[0].BaseObject as Hashtable;

            Assert.True(settingsResult.ContainsKey("$schema"));
            Assert.True(settingsResult.ContainsKey("source"));
            Assert.True(settingsResult.ContainsKey("experimentalFeatures"));
            Assert.True(settingsResult.ContainsKey("visual"));
        }

        /// <summary>
        /// Test Set-WinGetUserSettings when the local settings file already have the schema property. It shouldn't
        /// be added twice.
        /// </summary>
        [Test]
        public void SetWinGetUserSettings_Schema()
        {
            var ogSettings = new Hashtable()
            {
                {
                    "$schema",
                    "https://aka.ms/winget-settings.schema.json"
                },
                {
                    "source",
                    new Hashtable()
                    {
                        { "autoUpdateIntervalInMinutes", 3 },
                    }
                },
            };

            WinGetSettingsHelper.SetWingetSettings(ogSettings);

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", true },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Set-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<Hashtable>(result[0].BaseObject);
            var settingsResult = result[0].BaseObject as Hashtable;

            Assert.True(settingsResult.ContainsKey("$schema"));
            Assert.False(settingsResult.ContainsKey("source"));
            Assert.True(settingsResult.ContainsKey("experimentalFeatures"));
        }

        /// <summary>
        /// Test Set-WinGetUserSettings when the local settings file is not a json.
        /// </summary>
        [Test]
        public void SetWinGetUserSettings_BadJsonFile()
        {
            WinGetSettingsHelper.SetWingetSettings("Hi, im not a json. Thank you, Test.");

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();
            var result = powerShellHost.PowerShell
                .AddCommand("Set-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .Invoke();

            Assert.That(result, Has.Exactly(1).Items);
            Assert.IsInstanceOf<Hashtable>(result[0].BaseObject);
            var settingsResult = result[0].BaseObject as Hashtable;

            Assert.True(settingsResult.ContainsKey("$schema"));
            Assert.True(settingsResult.ContainsKey("visual"));
        }

        /// <summary>
        /// Test Set-WinGetUserSettings when the local settings file is not a json.
        /// </summary>
        [Test]
        public void SetWinGetUserSettings_BadJsonFile_Merge()
        {
            WinGetSettingsHelper.SetWingetSettings("Hi, im not a json. Thank you, Test.");

            var inputSettings = new Hashtable()
            {
                {
                    "visual",
                    new Hashtable()
                    {
                        { "progressBar", "retro" },
                    }
                },
            };

            using var powerShellHost = new PowerShellHost();

            var cmdletException = Assert.Throws<CmdletInvocationException>(
                () => powerShellHost.PowerShell
                .AddCommand("Set-WinGetUserSettings")
                .AddParameter("UserSettings", inputSettings)
                .AddParameter("Merge")
                .Invoke());

            // If we reference Microsoft.WinGet.Client to this project PowerShell host fails with
            //   System.Management.Automation.CmdletInvocationException : Operation is not supported on this platform. (0x80131539)
            //   System.PlatformNotSupportedException : Operation is not supported on this platform. (0x80131539)
            // trying to load the runspace. This is most probably because the same dll is already loaded.
            // Check the type the long way.
            dynamic exception = cmdletException.InnerException;
            Assert.AreEqual(exception.GetType().ToString(), "Microsoft.WinGet.Client.Exceptions.UserSettingsReadException");
        }

        private bool IsRunning(string processName)
        {
            return Process.GetProcessesByName(processName).Length > 0;
        }
    }
}