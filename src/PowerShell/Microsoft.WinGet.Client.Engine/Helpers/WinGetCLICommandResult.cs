// -----------------------------------------------------------------------------
// <copyright file="WinGetCLICommandResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using Microsoft.WinGet.Client.Engine.Exceptions;

    /// <summary>
    /// Winget cli command result.
    /// </summary>
    internal class WinGetCLICommandResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetCLICommandResult"/> class.
        /// </summary>
        /// <param name="command">Command.</param>
        /// <param name="parameters">Parameters.</param>
        /// <param name="exitCode">Exit code.</param>
        /// <param name="stdOut">Standard output.</param>
        /// <param name="stdErr">Standard error.</param>
        public WinGetCLICommandResult(string command, string? parameters, int exitCode, string stdOut, string stdErr)
        {
            this.Command = command;
            this.Parameters = parameters;
            this.ExitCode = exitCode;
            this.StdOut = stdOut;
            this.StdErr = stdErr;
        }

        /// <summary>
        /// Gets the command.
        /// </summary>
        public string Command { get; private set; }

        /// <summary>
        /// Gets the parameters.
        /// </summary>
        public string? Parameters { get; private set; }

        /// <summary>
        /// Gets the exit code.
        /// </summary>
        public int ExitCode { get; private set; }

        /// <summary>
        /// Gets the standard output.
        /// </summary>
        public string StdOut { get; private set; }

        /// <summary>
        /// Gets the standard error.
        /// </summary>
        public string StdErr { get; private set; }

        /// <summary>
        /// Verifies exit code.
        /// </summary>
        /// <param name="exitCode">Optional exit code.</param>
        public void VerifyExitCode(int exitCode = 0)
        {
            if (this.ExitCode != exitCode)
            {
                throw new WinGetCLIException(
                    this.Command,
                    this.Parameters,
                    this.ExitCode,
                    this.StdOut,
                    this.StdErr);
            }
        }
    }
}
