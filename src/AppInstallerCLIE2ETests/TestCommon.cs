// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using Microsoft.Win32;
    using NUnit.Framework;
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Threading;

    public class TestCommon
    {
        public static string AICLIPath { get; set; }

        public static string AICLIPackagePath { get; set; }

        public static bool PackagedContext { get; set; }

        public static bool VerboseLogging { get; set; }

        public static bool LooseFileRegistration { get; set; }

        public static bool InvokeCommandInDesktopPackage { get; set; }

        public static string StaticFileRootPath { get; set; }

        public static string ExeInstallerPath { get; set; }

        public static string MsiInstallerPath { get; set; }

        public static string MsixInstallerPath { get; set; }

        public static string ZipInstallerPath { get; set; }
        
        public static string PackageCertificatePath { get; set; }

        public static string SettingsJsonFilePath {
            get
            {
                return PackagedContext ?
                    @"Packages\WinGetDevCLI_8wekyb3d8bbwe\LocalState\settings.json" :
                    @"Microsoft\WinGet\Settings\settings.json";
            }
        }

        public enum Scope
        {
            User,
            Machine
        }

        public struct RunCommandResult
        {
            public int ExitCode;
            public string StdOut;
            public string StdErr;
        }

        public static RunCommandResult RunAICLICommand(string command, string parameters, string stdIn = null, int timeOut = 60000)
        {
            string inputMsg =
                    "AICLI path: " + AICLIPath +
                    " Command: " + command +
                    " Parameters: " + parameters +
                    (string.IsNullOrEmpty(stdIn) ? "" : " StdIn: " + stdIn) +
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

        public static RunCommandResult RunAICLICommandViaDirectProcess(string command, string parameters, string stdIn = null, int timeOut = 60000)
        {
            RunCommandResult result = new RunCommandResult();
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

        // This method is used when the test is run in an OS that does not support AppExecutionAlias. E,g, our build machine.
        // There is not any existing API that'll activate a packaged app and wait for result, and not possible to capture the stdIn and stdOut.
        // This method tries to call Invoke-CommandInDesktopPackage PS command to make test executable run in packaged context.
        // Since Invoke-CommandInDesktopPackage just launches the executable and return, we use cmd pipe to get execution results.
        // The final constructed command will look like:
        //   Invoke-CommandInDesktopPackage ...... -Command cmd.exe -Args '-c <cmd command>'
        //   where <cmd command> will look like: "echo stdIn | appinst.exe args > stdout.txt 2> stderr.txt & echo %ERRORLEVEL% > exitcode.txt"
        // Then this method will read the piped result and return as RunCommandResult.
        public static RunCommandResult RunAICLICommandViaInvokeCommandInDesktopPackage(string command, string parameters, string stdIn = null, int timeOut = 60000)
        {
            string cmdCommandPiped = "";
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

            RunCommandResult result = new RunCommandResult();

            // Sometimes the files are still in use; allow for this with a wait and retry loop.
            for (int retryCount = 0; retryCount < 4; ++retryCount)
            {
                bool success = false;

                try
                {
                    result.ExitCode = File.Exists(exitCodeFile) ? int.Parse(File.ReadAllText(exitCodeFile).Trim()) : unchecked((int)0x80004005);
                    result.StdOut = File.Exists(stdOutFile) ? File.ReadAllText(stdOutFile) : "";
                    result.StdErr = File.Exists(stdErrFile) ? File.ReadAllText(stdErrFile) : "";
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

        public static RunCommandResult RunCommandWithResult(string fileName, string args, int timeOut = 60000)
        {
            TestContext.Out.WriteLine($"Running command: {fileName} {args}");

            Process p = new Process();
            p.StartInfo = new ProcessStartInfo(fileName, args);
            p.StartInfo.RedirectStandardOutput = true;
            p.StartInfo.RedirectStandardError = true;
            p.Start();

            RunCommandResult result = new RunCommandResult();
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

        public static string GetTestFile(string fileName)
        {
            return Path.Combine(TestContext.CurrentContext.TestDirectory, fileName);
        }

        public static string GetTestDataFile(string fileName)
        {
            return GetTestFile(Path.Combine("TestData", fileName));
        }

        public static string GetTestWorkDir()
        {
            string workDir = Path.Combine(TestContext.CurrentContext.TestDirectory, "WorkDirectory");
            Directory.CreateDirectory(workDir);
            return workDir;
        }

        public static string GetRandomTestDir()
        {
            string randDir = Path.Combine(GetTestWorkDir(), Path.GetRandomFileName());
            Directory.CreateDirectory(randDir);
            return randDir;
        }

        public static string GetRandomTestFile(string extension)
        {
            return Path.Combine(GetTestWorkDir(), Path.GetRandomFileName() + extension);
        }

        public static bool InstallMsix(string file)
        {
            return RunCommand("powershell", $"Add-AppxPackage \"{file}\"");
        }

        public static bool InstallMsixRegister(string packagePath)
        {
            string manifestFile = Path.Combine(packagePath, "AppxManifest.xml");
            return RunCommand("powershell", $"Add-AppxPackage -Register \"{manifestFile}\"");
        }

        public static bool RemoveMsix(string name)
        {
            return RunCommand("powershell", $"Get-AppxPackage \"{name}\" | Remove-AppxPackage");
        }

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

        public static string GetPortablePackagesDirectory()
        {
            return Path.Combine(Environment.GetEnvironmentVariable("LocalAppData"), "Microsoft", "WinGet", "Packages");
        }

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
                RunAICLICommand("uninstall", $"--product-code {productCode}");
            }

            Assert.AreEqual(shouldExist, exeExists, $"Expected portable exe path: {exePath}");
            Assert.AreEqual(shouldExist, symlinkExists, $"Expected portable symlink path: {symlinkPath}");
            Assert.AreEqual(shouldExist, portableEntryExists, $"Expected {productCode} subkey in path: {uninstallSubKey}");
            Assert.AreEqual(shouldExist, isAddedToPath, $"Expected path variable: {symlinkDirectory}");
        }

        /// <summary>
        /// Copies log files to the path %TEMP%\E2ETestLogs
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
        public static string GetTestServerCertificateHexString()
        {
            return Convert.ToHexString(File.ReadAllBytes(Path.Combine(StaticFileRootPath, Constants.TestSourceServerCertificateFileName)));
        }

        public static bool VerifyTestExeInstalledAndCleanup(string installDir, string expectedContent = null)
        {
            if (!File.Exists(Path.Combine(installDir, Constants.TestExeInstalledFileName)))
            {
                return false;
            }

            if (!string.IsNullOrEmpty(expectedContent))
            {
                string content = File.ReadAllText(Path.Combine(installDir, Constants.TestExeInstalledFileName));
                return content.Contains(expectedContent);
            }

            return RunCommand(Path.Combine(installDir, Constants.TestExeUninstallerFileName));
        }

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

        public static bool VerifyTestMsixInstalledAndCleanup()
        {
            var result = RunCommandWithResult("powershell", $"Get-AppxPackage {Constants.MsixInstallerName}");

            if (!result.StdOut.Contains(Constants.MsixInstallerName))
            {
                return false;
            }

            return RemoveMsix(Constants.MsixInstallerName);
        }

        public static bool VerifyTestExeUninstalled(string installDir)
        {
            return File.Exists(Path.Combine(installDir, Constants.TestExeUninstalledFileName));
        }

        public static bool VerifyTestMsiUninstalled(string installDir)
        {
            return !File.Exists(Path.Combine(installDir, Constants.AppInstallerTestExeInstallerExe));
        }

        public static bool VerifyTestMsixUninstalled()
        {
            var result = RunCommandWithResult("powershell", $"Get-AppxPackage {Constants.MsixInstallerName}");
            return string.IsNullOrWhiteSpace(result.StdOut);
        }

        public static void ModifyPortableARPEntryValue(string productCode, string name, string value)
        {
            const string uninstallSubKey = @"Software\Microsoft\Windows\CurrentVersion\Uninstall";
            using (RegistryKey uninstallRegistryKey = Registry.CurrentUser.OpenSubKey(uninstallSubKey, true))
            {
                RegistryKey entry = uninstallRegistryKey.OpenSubKey(productCode, true);
                entry.SetValue(name, value);
            }
        }

        public static void SetupTestSource(bool useGroupPolicyForTestSource = false)
        {
            TestCommon.RunAICLICommand("source reset", "--force");
            TestCommon.RunAICLICommand("source remove", Constants.DefaultWingetSourceName);
            TestCommon.RunAICLICommand("source remove", Constants.DefaultMSStoreSourceName);

            // TODO: If/when cert pinning is implemented on the packaged index source, useGroupPolicyForTestSource should be set to default true
            //       to enable testing it by default.  Until then, leaving this here...
            if (useGroupPolicyForTestSource)
            {
                GroupPolicyHelper.EnableAdditionalSources.SetEnabledList(new GroupPolicySource[]
                {
                    new GroupPolicySource
                    {
                        Name = Constants.TestSourceName,
                        Arg = Constants.TestSourceUrl,
                        Type = Constants.TestSourceType,
                        Data = Constants.TestSourceIdentifier,
                        Identifier = Constants.TestSourceIdentifier,
                        CertificatePinning = new GroupPolicyCertificatePinning
                        {
                            Chains = new GroupPolicyCertificatePinningChain[] {
                                new GroupPolicyCertificatePinningChain
                                {
                                    Chain = new GroupPolicyCertificatePinningDetails[]
                                    {
                                        new GroupPolicyCertificatePinningDetails
                                        {
                                            Validation = new string[] { "publickey" },
                                            EmbeddedCertificate = TestCommon.GetTestServerCertificateHexString()
                                        }
                                    }
                                }
                            }
                        }
                    }
                });
            }
            else
            {
                GroupPolicyHelper.EnableAdditionalSources.SetNotConfigured();
                TestCommon.RunAICLICommand("source add", $"{Constants.TestSourceName} {Constants.TestSourceUrl}");
            }

            Thread.Sleep(2000);
        }

        public static void TearDownTestSource()
        {
            RunAICLICommand("source remove", Constants.TestSourceName);
            RunAICLICommand("source reset", "--force");
        }
    }
}
