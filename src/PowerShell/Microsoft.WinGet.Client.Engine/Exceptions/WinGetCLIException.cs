// -----------------------------------------------------------------------------
// <copyright file="WinGetCLIException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System.Management.Automation;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// WinGet cli exception.
    /// </summary>
    public class WinGetCLIException : RuntimeException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetCLIException"/> class.
        /// </summary>
        /// <param name="command">Command.</param>
        /// <param name="parameters">Parameters.</param>
        /// <param name="exitCode">Exit code.</param>
        /// <param name="stdOut">Standard output.</param>
        /// <param name="stdErr">Standard error.</param>
        public WinGetCLIException(string command, string? parameters, int exitCode, string stdOut, string stdErr)
            : base(string.Format(Resources.WinGetCLIExceptionMessage, command, parameters, exitCode))
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
    }
}
