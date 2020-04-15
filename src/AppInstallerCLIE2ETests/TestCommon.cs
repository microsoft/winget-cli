// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System;
    using System.Diagnostics;
    using System.IO;

    public class TestCommon
    {
        public static string AICLIPath { get; set; }

        public static bool IsPackagedContext { get; set; }

        public static bool VerboseLogging { get; set; }

        public struct RunCommandResult
        {
            public int ExitCode;
            public string StdOut;
            public string StdErr;
        }

        public static RunCommandResult RunAICLICommand(string command, string parameters, string stdIn = null, int timeOut = 60000)
        {
            string inputMsg =
                    "Command: " + command +
                    " Parameters: " + parameters +
                    (string.IsNullOrEmpty(stdIn) ? "" : " StdIn: " + stdIn) +
                    " Timeout: " + timeOut;

            TestContext.Out.WriteLine("Started command run. " + inputMsg);

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
                    TestContext.Error.WriteLine("Command run error. " + inputMsg + " Error: " + result.StdErr);
                }

                if (VerboseLogging && !string.IsNullOrEmpty(result.StdOut))
                {
                    TestContext.Out.WriteLine("Command run output. " + inputMsg + " Output: " + result.StdOut);
                }
            }
            else
            {
                throw new TimeoutException("Command run timed out. " + inputMsg);
            }

            return result;
        }

        public static bool RunCommand(string fileName, string args, int timeOut = 60000)
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

        public static string GetRandomTestDir()
        {
            string randDir = Path.Combine(TestContext.CurrentContext.TestDirectory, Path.GetRandomFileName());
            Directory.CreateDirectory(randDir);
            return randDir;
        }

        public static bool InstallMsix(string file)
        {
            return RunCommand("powershell", $"Add-AppxPackage \"{file}\"");
        }

        public static bool RemoveMsix(string pfn)
        {
            return RunCommand("powershell", $"Remove-AppxPackage \"{pfn}\"");
        }
    }
}
