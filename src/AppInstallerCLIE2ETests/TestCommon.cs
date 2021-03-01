// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
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
        
        public static string PackageCertificatePath { get; set; }

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
                throw new TimeoutException("Command run timed out.");
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
                throw new TimeoutException("Command run timed out.");
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
            return RunCommandWithResult(fileName, args, timeOut).ExitCode == 0;
        }

        public static RunCommandResult RunCommandWithResult(string fileName, string args, int timeOut = 60000)
        {
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

        /// <summary>
        /// Copies log files to the path %TEMP%\E2ETestLogs
        /// </summary>
        public static void PublishE2ETestLogs()
        {
            string tempPath = Path.GetTempPath();
            string localAppDataPath = Environment.GetEnvironmentVariable("LocalAppData");
            string testLogsSourcePath = Path.Combine(localAppDataPath, Constants.E2ETestLogsPath);
            string testLogsDestPath = Path.Combine(tempPath, "E2ETestLogs");

            if (Directory.Exists(testLogsDestPath))
            {
                TestIndexSetup.DeleteDirectoryContents(new DirectoryInfo(testLogsDestPath));
                Directory.Delete(testLogsDestPath);
            }

            if (Directory.Exists(testLogsSourcePath))
            {
                TestIndexSetup.CopyDirectory(testLogsSourcePath, testLogsDestPath);
            }
        }
    }
}
