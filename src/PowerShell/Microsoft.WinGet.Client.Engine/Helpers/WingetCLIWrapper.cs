// -----------------------------------------------------------------------------
// <copyright file="WingetCLIWrapper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Common.Command;

    /// <summary>
    /// Calls winget directly.
    /// </summary>
    internal class WingetCLIWrapper
    {
        /// <summary>
        /// The file name to use in start info.
        /// </summary>
        private readonly string wingetPath;

        /// <summary>
        /// Initializes a new instance of the <see cref="WingetCLIWrapper"/> class.
        /// When app execution alias is disabled the path of the exe is
        /// in the package family name directory in the local app data windows app directory. If its enabled then there's
        /// link in the windows app data directory. To avoid checking if its enabled or not, just look in the package
        /// family name directory.
        /// For test, point to the wingetdev executable.
        /// </summary>
        /// <param name="fullPath">Use full path or not.</param>
        public WingetCLIWrapper(bool fullPath = true)
        {
            if (Utilities.ExecutingAsSystem)
            {
                throw new NotSupportedException();
            }

            if (fullPath)
            {
                this.wingetPath = WinGetFullPath;
            }
            else
            {
                this.wingetPath = Constants.WinGetExe;
            }
        }

        /// <summary>
        /// Gets the full path of winget executable.
        /// </summary>
        public static string WinGetFullPath
        {
            get
            {
                return Path.Combine(
                    Utilities.LocalDataWindowsAppPath,
                    Constants.WingetPackageFamilyName,
                    Constants.WinGetExe);
            }
        }

        /// <summary>
        /// Runs winget command with parameters.
        /// </summary>
        /// <param name="pwshCmdlet">PowerShell cmdlet.</param>
        /// <param name="command">Command.</param>
        /// <param name="parameters">Parameters.</param>
        /// <param name="timeOut">Time out.</param>
        /// <returns>WinGetCommandResult.</returns>
        public WinGetCLICommandResult RunCommand(PowerShellCmdlet pwshCmdlet, string command, string? parameters = null, int timeOut = 60000)
        {
            string args = command;
            if (!string.IsNullOrEmpty(parameters))
            {
                args += ' ' + parameters;
            }

            pwshCmdlet.Write(StreamType.Verbose, $"Running {this.wingetPath} with {args}");

            Process p = new ()
            {
                StartInfo = new (this.wingetPath, args)
                {
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                },
            };

            p.Start();

            if (p.WaitForExit(timeOut))
            {
                return new WinGetCLICommandResult(
                    command,
                    parameters,
                    p.ExitCode,
                    p.StandardOutput.ReadToEnd(),
                    p.StandardError.ReadToEnd());
            }

            throw new WinGetCLITimeoutException(command, parameters);
        }
    }
}
