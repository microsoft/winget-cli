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
        /// </summary>
        /// <param name="wingetPath">The full path to the WinGet executable.</param>
        public WingetCLIWrapper(string wingetPath)
        {
            this.wingetPath = wingetPath ?? throw new ArgumentNullException(nameof(wingetPath));

            if (Utilities.ExecutingAsSystem)
            {
                throw new NotSupportedException();
            }
        }

        /// <summary>
        /// Gets the WinGet app execution alias path in the package family directory.
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
        /// <param name="builder">The command builder.</param>
        /// <param name="timeOut">Time out.</param>
        /// <returns>WinGetCommandResult.</returns>
        internal WinGetCLICommandResult RunCommand(PowerShellCmdlet pwshCmdlet, WinGetCLICommandBuilder builder, int timeOut = 60000)
        {
            var args = builder.ToString();
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
                    builder.Command,
                    builder.Parameters,
                    p.ExitCode,
                    p.StandardOutput.ReadToEnd(),
                    p.StandardError.ReadToEnd());
            }

            throw new WinGetCLITimeoutException(builder.Command, builder.Parameters);
        }
    }
}
