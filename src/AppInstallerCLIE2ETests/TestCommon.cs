// -----------------------------------------------------------------------------
// <copyright file="TestCommon.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Threading;
    using Microsoft.Win32;
    using NUnit.Framework;

    /// <summary>
    /// Test common.
    /// </summary>
    public class TestCommon
    {
        /// <summary>
        /// Scope.
        /// </summary>
        public enum Scope
        {
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
        /// Gets or sets the cli path.
        /// </summary>
        public static string AICLIPath { get; set; }

        /// <summary>
        /// Gets or sets the package path.
        /// </summary>
        public static string AICLIPackagePath { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the test runs in package context.
        /// </summary>
        public static bool PackagedContext { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the test uses verbose logging.
        /// </summary>
        public static bool VerboseLogging { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to use loose file registration.
        /// </summary>
        public static bool LooseFileRegistration { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to invoke command in desktop package.
        /// </summary>
        public static bool InvokeCommandInDesktopPackage { get; set; }

        /// <summary>
        /// Gets or sets the static file root path.
        /// </summary>
        public static string StaticFileRootPath { get; set; }

        /// <summary>
        /// Gets or sets the exe installer path.
        /// </summary>
        public static string ExeInstallerPath { get; set; }

        /// <summary>
        /// Gets or sets the msi installer path.
        /// </summary>
        public static string MsiInstallerPath { get; set; }

        /// <summary>
        /// Gets or sets the msix installer path.
        /// </summary>
        public static string MsixInstallerPath { get; set; }

        /// <summary>
        /// Gets or sets the zip installer path.
        /// </summary>
        public static string ZipInstallerPath { get; set; }

        /// <summary>
        /// Gets or sets the package cert path.
        /// </summary>
        public static string PackageCertificatePath { get; set; }

        /// <summary>
        /// Gets or sets the PowerShell module path.
        /// </summary>
        public static string PowerShellModulePath { get; set; }

        /// <summary>
        /// Gets or sets the settings json path.
        /// </summary>
        public static string SettingsJsonFilePath { get; set; }

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
                    "AICLI path: " + AICLIPath +
                    " Command: " + command +
                    " Parameters: " + parameters +
                    (string.IsNullOrEmpty(stdIn) ? string.Empty : " StdIn: " + stdIn) +
                    " Timeout: " + timeOut;

            TestContext.Out.WriteLine($"Starting command run. {inputMsg} InvokeCommandInDesktopPackage: {InvokeCommandInDesktopPackage}");

            if (InvokeCommandInDesktopPackage)
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
            p.StartInfo = new ProcessStartInfo(AICLIPath, command + ' ' + parameters);
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

                if (VerboseLogging && !string.IsNullOrEmpty(result.StdOut))
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
        /// <returns>The result of the command.</returns>
        public static RunCommandResult RunAICLICommandViaInvokeCommandInDesktopPackage(string command, string parameters, string stdIn = null, int timeOut = 60000)
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
            cmdCommandPiped += $"chcp 65001\n{AICLIPath} {command} {parameters} > {stdOutFile} 2> {stdErrFile}\necho %ERRORLEVEL% > {exitCodeFile}";
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

            if (waitedTime >= timeOut)
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
        /// <returns>True if exit code is 0.</returns>
        public static bool RunCommand(string fileName, string args = "", int timeOut = 60000)
        {
            RunCommandResult result = RunCommandWithResult(fileName, args, timeOut);

            if (result.ExitCode != 0)
            {
                TestContext.Out.WriteLine($"Command failed with: {result.ExitCode}");
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

                if (VerboseLogging)
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
            return RunCommandWithResult("pwsh.exe", $"-Command ipmo {PowerShellModulePath}; {cmdlet} {args}", timeOut);
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
            return RunCommand("powershell", $"Add-AppxPackage \"{file}\"");
        }

        /// <summary>
        /// Install and register msix package via appx manifest.
        /// </summary>
        /// <param name="packagePath">Path to package.</param>
        /// <returns>True if installed correctly.</returns>
        public static bool InstallMsixRegister(string packagePath)
        {
            string manifestFile = Path.Combine(packagePath, "AppxManifest.xml");
            return RunCommand("powershell", $"Add-AppxPackage -Register \"{manifestFile}\"");
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
        /// Get portable symlink dir.
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
        /// Get portable package directory.
        /// </summary>
        /// <returns>The portable package directory.</returns>
        public static string GetPortablePackagesDirectory()
        {
            return Path.Combine(Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
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
            string exePath = Path.Combine(installDir, filename);
            bool exeExists = File.Exists(exePath);

            string symlinkDirectory = GetPortableSymlinkDirectory(scope);
            string symlinkPath = Path.Combine(symlinkDirectory, commandAlias);
            bool symlinkExists = File.Exists(symlinkPath);

            bool portableEntryExists;
            RegistryKey baseKey = (scope == Scope.User) ? Registry.CurrentUser : Registry.LocalMachine;
            string uninstallSubKey = Constants.UninstallSubKey;
            using (RegistryKey uninstallRegistryKey = baseKey.OpenSubKey(uninstallSubKey, true))
            {
                RegistryKey portableEntry = uninstallRegistryKey.OpenSubKey(productCode, true);
                portableEntryExists = portableEntry != null;
            }

            bool isAddedToPath;
            string pathSubKey = (scope == Scope.User) ? Constants.PathSubKey_User : Constants.PathSubKey_Machine;
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

            if (Directory.Exists(testLogsDestPath))
            {
                TestIndexSetup.DeleteDirectoryContents(new DirectoryInfo(testLogsDestPath));
                Directory.Delete(testLogsDestPath);
            }

            if (Directory.Exists(testLogsPackagedSourcePath))
            {
                TestIndexSetup.CopyDirectory(testLogsPackagedSourcePath, testLogsPackagedDestPath);
            }

            if (Directory.Exists(testLogsUnpackagedSourcePath))
            {
                TestIndexSetup.CopyDirectory(testLogsUnpackagedSourcePath, testLogsUnpackagedDestPath);
            }
        }

        /// <summary>
        /// Gets the server certificate as a hex string.
        /// </summary>
        /// <returns>Hex string.</returns>
        public static string GetTestServerCertificateHexString()
        {
            return Convert.ToHexString(File.ReadAllBytes(Path.Combine(StaticFileRootPath, Constants.TestSourceServerCertificateFileName)));
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
            const string uninstallSubKey = @"Software\Microsoft\Windows\CurrentVersion\Uninstall";
            using (RegistryKey uninstallRegistryKey = Registry.CurrentUser.OpenSubKey(uninstallSubKey, true))
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
            TestCommon.RunAICLICommand("source reset", "--force");
            TestCommon.RunAICLICommand("source remove", Constants.DefaultWingetSourceName);
            TestCommon.RunAICLICommand("source remove", Constants.DefaultMSStoreSourceName);

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
                                            EmbeddedCertificate = TestCommon.GetTestServerCertificateHexString(),
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
                TestCommon.RunAICLICommand("source add", $"{Constants.TestSourceName} {Constants.TestSourceUrl}");
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
