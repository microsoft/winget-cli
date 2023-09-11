﻿// -----------------------------------------------------------------------------
// <copyright file="TestCommon.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using System.Reflection;
    using System.Threading;
    using System.Xml.Linq;
    using AppInstallerCLIE2ETests;
    using AppInstallerCLIE2ETests.PowerShell;
    using Microsoft.Management.Deployment;
    using Microsoft.Win32;
    using NUnit.Framework;

    /// <summary>
    /// Test common.
    /// </summary>
    public static class TestCommon
    {
        /// <summary>
        /// Scope.
        /// </summary>
        public enum Scope
        {
            /// <summary>
            /// None.
            /// </summary>
            Unknown,

            /// <summary>
            /// User.
            /// </summary>
            User,

            /// <summary>
            /// Machine.
            /// </summary>
            Machine,
        }

        /// <summary>
        /// The type of location.
        /// </summary>
        public enum TestModuleLocation
        {
            /// <summary>
            /// Current user.
            /// </summary>
            CurrentUser,

            /// <summary>
            /// All users.
            /// </summary>
            AllUsers,

            /// <summary>
            /// Winget module path.
            /// </summary>
            WinGetModulePath,

            /// <summary>
            /// Custom.
            /// </summary>
            Custom,

            /// <summary>
            /// Default winget configure.
            /// </summary>
            Default,
        }

        /// <summary>
        /// Run winget command.
        /// </summary>
        /// <param name="command">Command to run.</param>
        /// <param name="parameters">Parameters.</param>
        /// <param name="stdIn">Optional std in.</param>
        /// <param name="timeOut">Optional timeout.</param>
        /// <returns>The result of the command.</returns>
        public static RunCommandResult RunAICLICommand(string command, string parameters, string stdIn = null, int timeOut = 60000)
        {
            string inputMsg =
                    "AICLI path: " + TestSetup.Parameters.AICLIPath +
                    " Command: " + command +
                    " Parameters: " + parameters +
                    (string.IsNullOrEmpty(stdIn) ? string.Empty : " StdIn: " + stdIn) +
                    " Timeout: " + timeOut;

            TestContext.Out.WriteLine($"Starting command run. {inputMsg} InvokeCommandInDesktopPackage: {TestSetup.Parameters.InvokeCommandInDesktopPackage}");

            if (TestSetup.Parameters.InvokeCommandInDesktopPackage)
            {
                return RunAICLICommandViaInvokeCommandInDesktopPackage(command, parameters, stdIn, timeOut);
            }
            else
            {
                return RunAICLICommandViaDirectProcess(command, parameters, stdIn, timeOut);
            }
        }

        /// <summary>
        /// Run winget command via direct process.
        /// </summary>
        /// <param name="command">Command to run.</param>
        /// <param name="parameters">Parameters.</param>
        /// <param name="stdIn">Optional std in.</param>
        /// <param name="timeOut">Optional timeout.</param>
        /// <returns>The result of the command.</returns>
        public static RunCommandResult RunAICLICommandViaDirectProcess(string command, string parameters, string stdIn = null, int timeOut = 60000)
        {
            RunCommandResult result = new ();
            Process p = new Process();
            p.StartInfo = new ProcessStartInfo(TestSetup.Parameters.AICLIPath, command + ' ' + parameters);
            p.StartInfo.UseShellExecute = false;
            p.StartInfo.RedirectStandardOutput = true;
            p.StartInfo.RedirectStandardError = true;

            if (!string.IsNullOrEmpty(stdIn))
            {
                p.StartInfo.RedirectStandardInput = true;
            }

            p.Start();

            if (!string.IsNullOrEmpty(stdIn))
            {
                p.StandardInput.Write(stdIn);
            }

            if (p.WaitForExit(timeOut))
            {
                result.ExitCode = p.ExitCode;
                result.StdOut = p.StandardOutput.ReadToEnd();
                result.StdErr = p.StandardError.ReadToEnd();

                TestContext.Out.WriteLine("Command run completed with exit code: " + result.ExitCode);

                if (!string.IsNullOrEmpty(result.StdErr))
                {
                    TestContext.Error.WriteLine("Command run error. Error: " + result.StdErr);
                }

                if (TestSetup.Parameters.VerboseLogging && !string.IsNullOrEmpty(result.StdOut))
                {
                    TestContext.Out.WriteLine("Command run output. Output: " + result.StdOut);
                }
            }
            else
            {
                throw new TimeoutException($"Direct winget command run timed out: {command} {parameters}");
            }

            return result;
        }

        /// <summary>
        /// This method is used when the test is run in an OS that does not support AppExecutionAlias. E,g, our build machine.
        /// There is not any existing API that'll activate a packaged app and wait for result, and not possible to capture the stdIn and stdOut.
        /// This method tries to call Invoke-CommandInDesktopPackage PS command to make test executable run in packaged context.
        /// Since Invoke-CommandInDesktopPackage just launches the executable and return, we use cmd pipe to get execution results.
        /// The final constructed command will look like:
        ///   Invoke-CommandInDesktopPackage ...... -Command cmd.exe -Args '-c [cmd command]'
        ///   where [cmd command] will look like: "echo stdIn | appinst.exe args > stdout.txt 2> stderr.txt &amp;amp; echo %ERRORLEVEL% > exitcode.txt"
        /// Then this method will read the piped result and return as RunCommandResult.
        /// </summary>
        /// <param name="command">Command to run.</param>
        /// <param name="parameters">Parameters.</param>
        /// <param name="stdIn">Optional std in.</param>
        /// <param name="timeOut">Optional timeout.</param>
        /// <param name="throwOnTimeout">Throw on timeout.</param>
        /// <returns>The result of the command.</returns>
        public static RunCommandResult RunAICLICommandViaInvokeCommandInDesktopPackage(string command, string parameters, string stdIn = null, int timeOut = 60000, bool throwOnTimeout = true)
        {
            string cmdCommandPiped = string.Empty;
            if (!string.IsNullOrEmpty(stdIn))
            {
                cmdCommandPiped += $"echo {stdIn} | ";
            }

            string workDirectory = GetRandomTestDir();
            string tempBatchFile = Path.Combine(workDirectory, "Batch.cmd");
            string exitCodeFile = Path.Combine(workDirectory, "ExitCode.txt");
            string stdOutFile = Path.Combine(workDirectory, "StdOut.txt");
            string stdErrFile = Path.Combine(workDirectory, "StdErr.txt");

            // First change the codepage so that the rest of the batch file works
            cmdCommandPiped += $"chcp 65001\n{TestSetup.Parameters.AICLIPath} {command} {parameters} > {stdOutFile} 2> {stdErrFile}\necho %ERRORLEVEL% > {exitCodeFile}";
            File.WriteAllText(tempBatchFile, cmdCommandPiped, new System.Text.UTF8Encoding(false));

            string psCommand = $"Invoke-CommandInDesktopPackage -PackageFamilyName {Constants.AICLIPackageFamilyName} -AppId {Constants.AICLIAppId} -PreventBreakaway -Command cmd.exe -Args '/c \"{tempBatchFile}\"'";

            var psInvokeResult = RunCommandWithResult("powershell", psCommand);

            if (psInvokeResult.ExitCode != 0)
            {
                // PS invocation failed, return result and no need to check piped output.
                return psInvokeResult;
            }

            // The PS command just launches the app and immediately returns, we'll have to wait for up to the timeOut specified here
            int waitedTime = 0;
            while (!File.Exists(exitCodeFile) && waitedTime <= timeOut)
            {
                Thread.Sleep(1000);
                waitedTime += 1000;
            }

            if (waitedTime >= timeOut && throwOnTimeout)
            {
                throw new TimeoutException($"Packaged winget command run timed out: {command} {parameters}");
            }

            RunCommandResult result = new ();

            // Sometimes the files are still in use; allow for this with a wait and retry loop.
            for (int retryCount = 0; retryCount < 4; ++retryCount)
            {
                bool success = false;

                try
                {
                    result.ExitCode = File.Exists(exitCodeFile) ? int.Parse(File.ReadAllText(exitCodeFile).Trim()) : unchecked((int)0x80004005);
                    result.StdOut = File.Exists(stdOutFile) ? File.ReadAllText(stdOutFile) : string.Empty;
                    result.StdErr = File.Exists(stdErrFile) ? File.ReadAllText(stdErrFile) : string.Empty;
                    success = true;
                }
                catch (Exception e)
                {
                    TestContext.Out.WriteLine("Failed to access files: " + e.Message);
                }

                if (success)
                {
                    break;
                }
                else
                {
                    Thread.Sleep(250);
                }
            }

            return result;
        }

        /// <summary>
        /// Run command.
        /// </summary>
        /// <param name="fileName">File name.</param>
        /// <param name="args">Args.</param>
        /// <param name="timeOut">Time out.</param>
        /// <param name="throwOnFailure">If true, throw instead of returning false on a failure.</param>
        /// <returns>True if exit code is 0.</returns>
        public static bool RunCommand(string fileName, string args = "", int timeOut = 60000, bool throwOnFailure = false)
        {
            RunCommandResult result = RunCommandWithResult(fileName, args, timeOut);

            if (result.ExitCode != 0)
            {
                TestContext.Out.WriteLine($"Command failed with: {result.ExitCode}");
                if (throwOnFailure)
                {
                    throw new RunCommandException(fileName, args, result);
                }

                return false;
            }
            else
            {
                return true;
            }
        }

        /// <summary>
        /// Run command with result.
        /// </summary>
        /// <param name="fileName">File name.</param>
        /// <param name="args">Args.</param>
        /// <param name="timeOut">Optional timeout.</param>
        /// <returns>Command result.</returns>
        public static RunCommandResult RunCommandWithResult(string fileName, string args, int timeOut = 60000)
        {
            TestContext.Out.WriteLine($"Running command: {fileName} {args}");

            Process p = new Process();
            p.StartInfo = new ProcessStartInfo(fileName, args);
            p.StartInfo.RedirectStandardOutput = true;
            p.StartInfo.RedirectStandardError = true;
            p.Start();

            RunCommandResult result = new ();
            if (p.WaitForExit(timeOut))
            {
                result.ExitCode = p.ExitCode;
                result.StdOut = p.StandardOutput.ReadToEnd();
                result.StdErr = p.StandardError.ReadToEnd();

                if (TestSetup.Parameters.VerboseLogging)
                {
                    TestContext.Out.WriteLine($"Command run finished. {fileName} {args} {timeOut}. Output: {result.StdOut} Error: {result.StdErr}");
                }
            }
            else
            {
                throw new TimeoutException($"Command run timed out. {fileName} {args} {timeOut}");
            }

            return result;
        }

        /// <summary>
        /// Run PowerShell Core command with result.
        /// </summary>
        /// <param name="cmdlet">Cmdlet to run.</param>
        /// <param name="args">Args.</param>
        /// <param name="timeOut">Optional timeout.</param>
        /// <returns>Command result.</returns>
        public static RunCommandResult RunPowerShellCoreCommandWithResult(string cmdlet, string args, int timeOut = 60000)
        {
            return RunCommandWithResult("pwsh.exe", $"-Command ipmo {TestSetup.Parameters.PowerShellModuleManifestPath}; {cmdlet} {args}", timeOut);
        }

        /// <summary>
        /// Get test file path.
        /// </summary>
        /// <param name="fileName">Test file name.</param>
        /// <returns>Path of test file.</returns>
        public static string GetTestFile(string fileName)
        {
            return Path.Combine(TestContext.CurrentContext.TestDirectory, fileName);
        }

        /// <summary>
        /// Get test data file path.
        /// </summary>
        /// <param name="fileName">File name.</param>
        /// <returns>Test file data path.</returns>
        public static string GetTestDataFile(string fileName)
        {
            return GetTestFile(Path.Combine("TestData", fileName));
        }

        /// <summary>
        /// Get test work directory. Creates if not exists.
        /// </summary>
        /// <returns>The work directory.</returns>
        public static string GetTestWorkDir()
        {
            string workDir = Path.Combine(TestContext.CurrentContext.TestDirectory, "WorkDirectory");
            Directory.CreateDirectory(workDir);
            return workDir;
        }

        /// <summary>
        /// Create random test directory.
        /// </summary>
        /// <returns>Path of new test directory.</returns>
        public static string GetRandomTestDir()
        {
            string randDir = Path.Combine(GetTestWorkDir(), Path.GetRandomFileName());
            Directory.CreateDirectory(randDir);
            return randDir;
        }

        /// <summary>
        /// Creates new random file name. File is not created.
        /// </summary>
        /// <param name="extension">Extension of random file.</param>
        /// <returns>Path of random file.</returns>
        public static string GetRandomTestFile(string extension)
        {
            return Path.Combine(GetTestWorkDir(), Path.GetRandomFileName() + extension);
        }

        /// <summary>
        /// Install msix package via PowerShell.
        /// </summary>
        /// <param name="file">Msix file.</param>
        /// <returns>True if installed.</returns>
        public static bool InstallMsix(string file)
        {
            return RunCommand("powershell", $"Add-AppxPackage \"{file}\"", throwOnFailure: true);
        }

        /// <summary>
        /// Install and register msix package via appx manifest.
        /// </summary>
        /// <param name="packagePath">Path to package.</param>
        /// <param name="forceShutdown">Force shutdown.</param>
        /// <param name="throwOnFailure">Throw on failure.</param>
        /// <returns>True if installed correctly.</returns>
        public static bool InstallMsixRegister(string packagePath, bool forceShutdown = false, bool throwOnFailure = true)
        {
            string manifestFile = Path.Combine(packagePath, "AppxManifest.xml");

            var command = $"Add-AppxPackage -Register \"{manifestFile}\"";
            if (forceShutdown)
            {
                command += " -ForceTargetApplicationShutdown";
            }

            return RunCommand("powershell", command, throwOnFailure: throwOnFailure);
        }

        /// <summary>
        /// Remove msix package.
        /// </summary>
        /// <param name="name">Package to remove.</param>
        /// <param name="isProvisioned">Whether the package is provisioned.</param>
        /// <returns>True if removed correctly.</returns>
        public static bool RemoveMsix(string name, bool isProvisioned = false)
        {
            if (isProvisioned)
            {
                return RunCommand("powershell", $"Get-AppxProvisionedPackage -Online | Where-Object {{$_.PackageName -like \"*{name}*\"}} | Remove-AppxProvisionedPackage -Online -AllUsers") &&
                    RunCommand("powershell", $"Get-AppxPackage \"{name}\" | Remove-AppxPackage -AllUsers");
            }
            else
            {
                return RunCommand("powershell", $"Get-AppxPackage \"{name}\" | Remove-AppxPackage");
            }
        }

        /// <summary>
        /// Gets the portable symlink directory.
        /// </summary>
        /// <param name="scope">Scope.</param>
        /// <returns>The path of the symlinks.</returns>
        public static string GetPortableSymlinkDirectory(Scope scope)
        {
            if (scope == Scope.User)
            {
                return Path.Combine(Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Links");
            }
            else
            {
                return Path.Combine(Environment.GetEnvironmentVariable("ProgramFiles"), "WinGet", "Links");
            }
        }

        /// <summary>
        /// Gets the portable package directory.
        /// </summary>
        /// <returns>The portable package directory.</returns>
        public static string GetPortablePackagesDirectory()
        {
            return Path.Combine(Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
        }

        /// <summary>
        /// Gets the default download directory for the download command.
        /// </summary>
        /// <returns>The default download directory.</returns>
        public static string GetDefaultDownloadDirectory()
        {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "Downloads");
        }

        /// <summary>
        /// Verify portable package.
        /// </summary>
        /// <param name="installDir">Install dir.</param>
        /// <param name="commandAlias">Command alias.</param>
        /// <param name="filename">File name.</param>
        /// <param name="productCode">Product code.</param>
        /// <param name="shouldExist">Should exists.</param>
        /// <param name="scope">Scope.</param>
        public static void VerifyPortablePackage(
            string installDir,
            string commandAlias,
            string filename,
            string productCode,
            bool shouldExist,
            Scope scope = Scope.User)
        {
            // When portables are installed, if the exe path is inside a directory it will not be aliased
            // if the exe path is at the root level, it will be aliased. Therefore, if either exist, the exe exists
            string exePath = Path.Combine(installDir, filename);
            string exeAliasedPath = Path.Combine(installDir, commandAlias);
            bool exeExists = File.Exists(exePath) || File.Exists(exeAliasedPath);

            string symlinkDirectory = GetPortableSymlinkDirectory(scope);
            string symlinkPath = Path.Combine(symlinkDirectory, commandAlias);
            bool symlinkExists = File.Exists(symlinkPath);

            bool portableEntryExists;
            RegistryKey baseKey = scope == Scope.User ? Registry.CurrentUser : Registry.LocalMachine;
            string uninstallSubKey = Constants.UninstallSubKey;
            using (RegistryKey uninstallRegistryKey = baseKey.OpenSubKey(uninstallSubKey, true))
            {
                RegistryKey portableEntry = uninstallRegistryKey.OpenSubKey(productCode, true);
                portableEntryExists = portableEntry != null;
            }

            bool isAddedToPath;
            string pathSubKey = scope == Scope.User ? Constants.PathSubKey_User : Constants.PathSubKey_Machine;
            using (RegistryKey environmentRegistryKey = baseKey.OpenSubKey(pathSubKey, true))
            {
                string pathName = "Path";
                var currentPathValue = (string)environmentRegistryKey.GetValue(pathName);
                var portablePathValue = symlinkDirectory + ';';
                isAddedToPath = currentPathValue.Contains(portablePathValue);
            }

            if (shouldExist)
            {
                RunAICLICommand("uninstall", $"--product-code {productCode} --force");
            }

            Assert.AreEqual(shouldExist, exeExists, $"Expected portable exe path: {exePath}");
            Assert.AreEqual(shouldExist, symlinkExists, $"Expected portable symlink path: {symlinkPath}");
            Assert.AreEqual(shouldExist, portableEntryExists, $"Expected {productCode} subkey in path: {uninstallSubKey}");
            Assert.AreEqual(shouldExist, isAddedToPath, $"Expected path variable: {symlinkDirectory}");
        }

        /// <summary>
        /// Copies log files to the path %TEMP%\E2ETestLogs.
        /// </summary>
        public static void PublishE2ETestLogs()
        {
            string tempPath = Path.GetTempPath();
            string localAppDataPath = Environment.GetEnvironmentVariable("LocalAppData");
            string testLogsPackagedSourcePath = Path.Combine(localAppDataPath, Constants.E2ETestLogsPathPackaged);
            string testLogsUnpackagedSourcePath = Path.Combine(tempPath, Constants.E2ETestLogsPathUnpackaged);
            string testLogsDestPath = Path.Combine(tempPath, "E2ETestLogs");
            string testLogsPackagedDestPath = Path.Combine(testLogsDestPath, "Packaged");
            string testLogsUnpackagedDestPath = Path.Combine(testLogsDestPath, "Unpackaged");

            if (Directory.Exists(testLogsPackagedSourcePath))
            {
                CopyDirectory(testLogsPackagedSourcePath, testLogsPackagedDestPath);
            }

            if (Directory.Exists(testLogsUnpackagedSourcePath))
            {
                CopyDirectory(testLogsUnpackagedSourcePath, testLogsUnpackagedDestPath);
            }
        }

        /// <summary>
        /// Gets the server certificate as a hex string.
        /// </summary>
        /// <returns>Hex string.</returns>
        public static string GetTestServerCertificateHexString()
        {
            if (string.IsNullOrEmpty(TestSetup.Parameters.LocalServerCertPath))
            {
                throw new Exception($"{Constants.LocalServerCertPathParameter} not set.");
            }

            if (!File.Exists(TestSetup.Parameters.LocalServerCertPath))
            {
                throw new FileNotFoundException(TestSetup.Parameters.LocalServerCertPath);
            }

            return Convert.ToHexString(File.ReadAllBytes(TestSetup.Parameters.LocalServerCertPath));
        }

        /// <summary>
        /// Verify exe installer correctly.
        /// </summary>
        /// <param name="installDir">Install directory.</param>
        /// <param name="expectedContent">Optional expected content.</param>
        /// <returns>True if success.</returns>
        public static bool VerifyTestExeInstalled(string installDir, string expectedContent = null)
        {
            bool verifyInstallSuccess = true;

            if (!File.Exists(Path.Combine(installDir, Constants.TestExeInstalledFileName)))
            {
                TestContext.Out.WriteLine($"TestExeInstalled.exe not found at {installDir}");
                verifyInstallSuccess = false;
            }

            if (verifyInstallSuccess && !string.IsNullOrEmpty(expectedContent))
            {
                string content = File.ReadAllText(Path.Combine(installDir, Constants.TestExeInstalledFileName));
                TestContext.Out.WriteLine($"TestExeInstalled.exe content: {content}");
                verifyInstallSuccess = content.Contains(expectedContent);
            }

            return verifyInstallSuccess;
        }

        /// <summary>
        /// Verify installer and manifest downloaded correctly and cleanup.
        /// </summary>
        /// <param name="downloadDir">Download directory.</param>
        /// <param name="name">Package name.</param>
        /// <param name="version">Package version.</param>
        /// <param name="arch">Installer architecture.</param>
        /// <param name="scope">Installer scope.</param>
        /// <param name="installerType">Installer type.</param>
        /// <param name="locale">Installer locale.</param>
        /// <param name="isArchive">Boolean value indicating whether the installer is an archive.</param>
        /// <param name="cleanup">Boolean value indicating whether to remove the installer file and directory.</param>
        /// <returns>True if success.</returns>
        public static bool VerifyInstallerDownload(
            string downloadDir,
            string name,
            string version,
            Windows.System.ProcessorArchitecture arch,
            Scope scope,
            PackageInstallerType installerType,
            string locale = null,
            bool isArchive = false,
            bool cleanup = true)
        {
            string expectedFileName = $"{name}_{version}";

            if (scope != Scope.Unknown)
            {
                expectedFileName += $"_{scope}";
            }

            expectedFileName += $"_{arch}_{installerType}";

            if (!string.IsNullOrEmpty(locale))
            {
                expectedFileName += $"_{locale}";
            }

            string installerExtension;
            if (isArchive)
            {
                installerExtension = ".zip";
            }
            else
            {
                installerExtension = installerType switch
                {
                    PackageInstallerType.Msi => ".msi",
                    PackageInstallerType.Msix => ".msix",
                    _ => ".exe"
                };
            }

            string installerDownloadPath = Path.Combine(downloadDir, expectedFileName + installerExtension);
            string manifestDownloadPath = Path.Combine(downloadDir, expectedFileName + ".yaml");

            bool downloadResult = false;

            if (Directory.Exists(downloadDir) && File.Exists(installerDownloadPath) && File.Exists(manifestDownloadPath))
            {
                downloadResult = true;

                if (cleanup)
                {
                    File.Delete(installerDownloadPath);
                    File.Delete(manifestDownloadPath);
                    Directory.Delete(downloadDir, true);
                }
            }

            return downloadResult;
        }

        /// <summary>
        /// Verify exe installer correctly and then uninstall it.
        /// </summary>
        /// <param name="installDir">Install directory.</param>
        /// <param name="expectedContent">Optional expected content.</param>
        /// <returns>True if success.</returns>
        public static bool VerifyTestExeInstalledAndCleanup(string installDir, string expectedContent = null)
        {
            bool verifyInstallSuccess = VerifyTestExeInstalled(installDir, expectedContent);

            // Always try clean up and ignore clean up failure
            var uninstallerPath = Path.Combine(installDir, Constants.TestExeUninstallerFileName);
            if (File.Exists(uninstallerPath))
            {
                RunCommand(Path.Combine(installDir, Constants.TestExeUninstallerFileName));
            }

            return verifyInstallSuccess;
        }

        /// <summary>
        /// Verify msi installed correctly.
        /// </summary>
        /// <param name="installDir">Installed directory.</param>
        /// <returns>True if success.</returns>
        public static bool VerifyTestMsiInstalledAndCleanup(string installDir)
        {
            string pathToCheck = Path.Combine(installDir, Constants.AppInstallerTestExeInstallerExe);
            if (!File.Exists(pathToCheck))
            {
                TestContext.Out.WriteLine($"File not found: {pathToCheck}");
                return false;
            }

            return RunCommand("msiexec.exe", $"/qn /x {Constants.MsiInstallerProductCode}");
        }

        /// <summary>
        /// Verify msix installed correctly.
        /// </summary>
        /// <param name="isProvisioned">Whether the package is provisioned.</param>
        /// <returns>True if success.</returns>
        public static bool VerifyTestMsixInstalledAndCleanup(bool isProvisioned = false)
        {
            var result = RunCommandWithResult("powershell", $"Get-AppxPackage {Constants.MsixInstallerName}");

            if (!result.StdOut.Contains(Constants.MsixInstallerName))
            {
                return false;
            }

            if (isProvisioned)
            {
                result = RunCommandWithResult("powershell", $"Get-AppxProvisionedPackage -Online | Where-Object {{$_.PackageName -like \"*{Constants.MsixInstallerName}*\"}}");
                if (!result.StdOut.Contains(Constants.MsixInstallerName))
                {
                    return false;
                }
            }

            return RemoveMsix(Constants.MsixInstallerName, isProvisioned);
        }

        /// <summary>
        /// Verify test exe uninstalled.
        /// </summary>
        /// <param name="installDir">Installed directory.</param>
        /// <returns>True if success.</returns>
        public static bool VerifyTestExeUninstalled(string installDir)
        {
            return File.Exists(Path.Combine(installDir, Constants.TestExeUninstalledFileName));
        }

        /// <summary>
        /// Verify msi uninstalled.
        /// </summary>
        /// <param name="installDir">Install directory.</param>
        /// <returns>True if success.</returns>
        public static bool VerifyTestMsiUninstalled(string installDir)
        {
            return !File.Exists(Path.Combine(installDir, Constants.AppInstallerTestExeInstallerExe));
        }

        /// <summary>
        /// Verify msix uninstalled.
        /// </summary>
        /// <param name="isProvisioned">Whether the package is provisioned.</param>
        /// <returns>True if success.</returns>
        public static bool VerifyTestMsixUninstalled(bool isProvisioned = false)
        {
            bool isUninstalled = false;
            var result = RunCommandWithResult("powershell", $"Get-AppxPackage {Constants.MsixInstallerName}");
            isUninstalled = string.IsNullOrWhiteSpace(result.StdOut);

            if (isProvisioned)
            {
                result = RunCommandWithResult("powershell", $"Get-AppxProvisionedPackage -Online | Where-Object {{$_.PackageName -like \"*{Constants.MsixInstallerName}*\"}}");
                isUninstalled = isUninstalled && string.IsNullOrWhiteSpace(result.StdOut);
            }

            return isUninstalled;
        }

        /// <summary>
        /// Modify uninstalled registry key.
        /// </summary>
        /// <param name="productCode">Product code.</param>
        /// <param name="name">Name.</param>
        /// <param name="value">Value.</param>
        public static void ModifyPortableARPEntryValue(string productCode, string name, string value)
        {
            using (RegistryKey uninstallRegistryKey = Registry.CurrentUser.OpenSubKey(Constants.UninstallSubKey, true))
            {
                RegistryKey entry = uninstallRegistryKey.OpenSubKey(productCode, true);
                entry.SetValue(name, value);
            }
        }

        /// <summary>
        /// Set up test source.
        /// </summary>
        /// <param name="useGroupPolicyForTestSource">Use group policy.</param>
        public static void SetupTestSource(bool useGroupPolicyForTestSource = false)
        {
            RunAICLICommand("source reset", "--force");
            RunAICLICommand("source remove", Constants.DefaultWingetSourceName);
            RunAICLICommand("source remove", Constants.DefaultMSStoreSourceName);

            // TODO: If/when cert pinning is implemented on the packaged index source, useGroupPolicyForTestSource should be set to default true
            //       to enable testing it by default.  Until then, leaving this here...
            if (useGroupPolicyForTestSource)
            {
                GroupPolicyHelper.EnableAdditionalSources.SetEnabledList(new GroupPolicyHelper.GroupPolicySource[]
                {
                    new GroupPolicyHelper.GroupPolicySource
                    {
                        Name = Constants.TestSourceName,
                        Arg = Constants.TestSourceUrl,
                        Type = Constants.TestSourceType,
                        Data = Constants.TestSourceIdentifier,
                        Identifier = Constants.TestSourceIdentifier,
                        CertificatePinning = new GroupPolicyHelper.GroupPolicyCertificatePinning
                        {
                            Chains = new GroupPolicyHelper.GroupPolicyCertificatePinningChain[]
                            {
                                new GroupPolicyHelper.GroupPolicyCertificatePinningChain
                                {
                                    Chain = new GroupPolicyHelper.GroupPolicyCertificatePinningDetails[]
                                    {
                                        new GroupPolicyHelper.GroupPolicyCertificatePinningDetails
                                        {
                                            Validation = new string[] { "publickey" },
                                            EmbeddedCertificate = GetTestServerCertificateHexString(),
                                        },
                                    },
                                },
                            },
                        },
                    },
                });
            }
            else
            {
                GroupPolicyHelper.EnableAdditionalSources.SetNotConfigured();
                RunAICLICommand("source add", $"{Constants.TestSourceName} {Constants.TestSourceUrl}");
            }

            Thread.Sleep(2000);
        }

        /// <summary>
        /// Tear down test source.
        /// </summary>
        public static void TearDownTestSource()
        {
            RunAICLICommand("source remove", Constants.TestSourceName);
            RunAICLICommand("source reset", "--force");
        }

        /// <summary>
        /// Ensures that a module is in the desired state.
        /// </summary>
        /// <param name="moduleName">The module.</param>
        /// <param name="present">Whether the module is present or not.</param>
        /// <param name="repository">The repository to get the module from if needed.</param>
        /// <param name="location">The location to install the module.</param>
        public static void EnsureModuleState(string moduleName, bool present, string repository = null, TestCommon.TestModuleLocation location = TestModuleLocation.CurrentUser)
        {
            string wingetModulePath = TestCommon.GetExpectedModulePath(TestModuleLocation.WinGetModulePath);
            string customPath = TestCommon.GetExpectedModulePath(TestModuleLocation.Custom);

            ICollection<PSModuleInfo> e2eModule;
            bool isPresent = false;
            {
                using var pwsh = new PowerShellHost(false);
                pwsh.AddModulePath($"{wingetModulePath};{customPath}");

                e2eModule = pwsh.PowerShell.AddCommand("Get-Module").AddParameter("Name", moduleName).AddParameter("ListAvailable").Invoke<PSModuleInfo>();
                isPresent = e2eModule.Any();
            }

            if (isPresent)
            {
                // If the module was saved in a different location we can't Uninstall-Module.
                foreach (var module in e2eModule)
                {
                    var moduleBase = module.Path;
                    while (Path.GetFileName(moduleBase) != moduleName)
                    {
                        moduleBase = Path.GetDirectoryName(moduleBase);
                    }

                    if (!present)
                    {
                        Directory.Delete(moduleBase, true);
                    }
                    else
                    {
                        // Must be present in the right location.
                        var expectedLocation = TestCommon.GetExpectedModulePath(location);
                        if (!moduleBase.StartsWith(expectedLocation))
                        {
                            Directory.Delete(moduleBase, true);
                            isPresent = false;
                        }
                    }
                }
            }

            if (!isPresent && present)
            {
                if (location == TestModuleLocation.CurrentUser ||
                    location == TestModuleLocation.AllUsers)
                {
                    using var pwsh = new PowerShellHost(false);
                    pwsh.AddModulePath($"{wingetModulePath};{customPath}");
                    pwsh.PowerShell.AddCommand("Install-Module").AddParameter("Name", moduleName).AddParameter("Force");

                    if (!string.IsNullOrEmpty(repository))
                    {
                        pwsh.PowerShell.AddParameter("Repository", repository);
                    }

                    if (location == TestModuleLocation.AllUsers)
                    {
                        pwsh.PowerShell.AddParameter("Scope", "AllUsers");
                    }

                    _ = pwsh.PowerShell.Invoke();
                }
                else
                {
                    string path = customPath;
                    if (location == TestModuleLocation.WinGetModulePath ||
                        location == TestModuleLocation.Default)
                    {
                        path = wingetModulePath;
                    }

                    using var pwsh = new PowerShellHost(false);
                    pwsh.AddModulePath($"{wingetModulePath};{customPath}");
                    pwsh.PowerShell.AddCommand("Save-Module").AddParameter("Name", moduleName).AddParameter("Path", path).AddParameter("Force");

                    if (!string.IsNullOrEmpty(repository))
                    {
                        pwsh.PowerShell.AddParameter("Repository", repository);
                    }

                    _ = pwsh.PowerShell.Invoke();
                }
            }
        }

        /// <summary>
        /// Creates an ARP entry from the given values.
        /// </summary>
        /// <param name="productCode">Product code of the entry.</param>
        /// <param name="properties">The properties to set in the entry.</param>
        /// <param name="scope">Scope of the entry.</param>
        public static void CreateARPEntry(
            string productCode,
            object properties,
            Scope scope = Scope.User)
        {
            RegistryKey baseKey = scope == Scope.User ? Registry.CurrentUser : Registry.LocalMachine;
            using (RegistryKey uninstallRegistryKey = baseKey.OpenSubKey(Constants.UninstallSubKey, true))
            {
                RegistryKey entry = uninstallRegistryKey.CreateSubKey(productCode, true);

                foreach (PropertyInfo property in properties.GetType().GetProperties())
                {
                    entry.SetValue(property.Name, property.GetValue(properties));
                }
            }
        }

        /// <summary>
        /// Removes an ARP entry.
        /// </summary>
        /// <param name="productCode">Product code of the entry.</param>
        /// <param name="scope">Scope of the entry.</param>
        public static void RemoveARPEntry(
            string productCode,
            Scope scope = Scope.User)
        {
            RegistryKey baseKey = scope == Scope.User ? Registry.CurrentUser : Registry.LocalMachine;
            using (RegistryKey uninstallRegistryKey = baseKey.OpenSubKey(Constants.UninstallSubKey, true))
            {
                uninstallRegistryKey.DeleteSubKey(productCode);
            }
        }

        /// <summary>
        /// Copies the contents of a given directory from a source path to a destination path.
        /// </summary>
        /// <param name="sourceDirName">Source directory name.</param>
        /// <param name="destDirName">Destination directory name.</param>
        public static void CopyDirectory(string sourceDirName, string destDirName)
        {
            DirectoryInfo dir = new DirectoryInfo(sourceDirName);
            DirectoryInfo[] dirs = dir.GetDirectories();

            if (!Directory.Exists(destDirName))
            {
                Directory.CreateDirectory(destDirName);
            }

            FileInfo[] files = dir.GetFiles();
            foreach (FileInfo file in files)
            {
                string temppath = Path.Combine(destDirName, file.Name);
                file.CopyTo(temppath, false);
            }

            foreach (DirectoryInfo subdir in dirs)
            {
                string temppath = Path.Combine(destDirName, subdir.Name);
                CopyDirectory(subdir.FullName, temppath);
            }
        }

        /// <summary>
        /// Gets the expected module path.
        /// </summary>
        /// <param name="location">Location.</param>
        /// <returns>The expected path of the module.</returns>
        public static string GetExpectedModulePath(TestModuleLocation location)
        {
            switch (location)
            {
                case TestModuleLocation.CurrentUser:
                    return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), @"PowerShell\Modules");
                case TestModuleLocation.AllUsers:
                    return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), @"PowerShell\Modules");
                case TestModuleLocation.WinGetModulePath:
                case TestModuleLocation.Default:
                    return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), @"Microsoft\WinGet\Configuration\Modules");
                case TestModuleLocation.Custom:
                    return Path.Combine(Path.GetTempPath(), "E2ECustomModules");
                default:
                    throw new ArgumentException(location.ToString());
            }
        }

        /// <summary>
        /// Run command result.
        /// </summary>
        public struct RunCommandResult
        {
            /// <summary>
            /// Exit code.
            /// </summary>
            public int ExitCode;

            /// <summary>
            /// StdOut.
            /// </summary>
            public string StdOut;

            /// <summary>
            /// StdErr.
            /// </summary>
            public string StdErr;
        }
    }
}
