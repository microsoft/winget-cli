// -----------------------------------------------------------------------------
// <copyright file="WingetCLIWrapper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Common
{
    using System;
    using System.Diagnostics;
    using System.IO;

    /// <summary>
    /// Calls winget directly.
    /// </summary>
    internal class WingetCLIWrapper
    {
        private static readonly string WingetCliPath;

        /// <summary>
        /// Initializes static members of the <see cref="WingetCLIWrapper"/> class.
        /// When app execution alias is disabled the path of the exe is
        /// in the package family name directory in the local app data windows app directory. If its enabled then there's
        /// link in the windows app data directory. To avoid checking if its enabled or not, just look in the package
        /// family name directory.
        /// For test, point to the wingetdev executable.
        /// </summary>
        static WingetCLIWrapper()
        {
            string windowsAppPath = Environment.ExpandEnvironmentVariables("%LOCALAPPDATA%\\Microsoft\\WindowsApps");
#if USE_PROD_CLSIDS
            WingetCliPath = Path.Combine(
                windowsAppPath,
                "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe",
                "winget.exe");
#else
            WingetCliPath = Path.Combine(
                windowsAppPath,
                "WinGetDevCLI_8wekyb3d8bbwe",
                "wingetdev.exe");
#endif
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WingetCLIWrapper"/> class.
        /// </summary>
        public WingetCLIWrapper()
        {
            if (!File.Exists(WingetCliPath))
            {
                // Winget is not installed.
                throw new FileNotFoundException(WingetCliPath);
            }
        }

        /// <summary>
        /// Runs winget command with parameters.
        /// </summary>
        /// <param name="command">Command.</param>
        /// <param name="parameters">Parameters.</param>
        /// <param name="timeOut">Time out.</param>
        /// <returns>WinGetCommandResult.</returns>
        public WinGetCommandResult RunCommand(string command, string parameters, int timeOut = 60000)
        {
            WinGetCommandResult result = new ();
            Process p = new ()
            {
                StartInfo = new (WingetCliPath, command + ' ' + parameters)
                {
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                },
            };

            p.Start();

            if (p.WaitForExit(timeOut))
            {
                result.ExitCode = p.ExitCode;
                result.StdOut = p.StandardOutput.ReadToEnd();
                result.StdErr = p.StandardError.ReadToEnd();
            }
            else
            {
                throw new TimeoutException($"Direct winget command run timed out: {command} {parameters}");
            }

            return result;
        }

        /// <summary>
        /// Winget command result.
        /// </summary>
        public struct WinGetCommandResult
        {
            /// <summary>
            /// Exit code.
            /// </summary>
            public int ExitCode = -1;

            /// <summary>
            /// StdOut.
            /// </summary>
            public string StdOut = string.Empty;

            /// <summary>
            /// StdErr.
            /// </summary>
            public string StdErr = string.Empty;

            /// <summary>
            /// Initializes a new instance of the <see cref="WinGetCommandResult"/> struct.
            /// </summary>
            public WinGetCommandResult()
            {
            }

            /// <summary>
            /// Verifies exit code and std error.
            /// </summary>
            public void VerifyExitCode()
            {
                if (this.ExitCode != 0 || !string.IsNullOrEmpty(this.StdErr))
                {
                    // TODO: new exception.
                    throw new Exception($"ExitCode: '{this.ExitCode}' StdErr {this.StdErr}");
                }
            }
        }
    }
}
