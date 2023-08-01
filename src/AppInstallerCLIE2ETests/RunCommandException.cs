// -----------------------------------------------------------------------------
// <copyright file="RunCommandException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using static AppInstallerCLIE2ETests.Helpers.TestCommon;

    /// <summary>
    /// An exception that occurred when running a command.
    /// </summary>
    internal class RunCommandException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="RunCommandException"/> class.
        /// </summary>
        /// <param name="fileName">The file name of the command.</param>
        /// <param name="args">The arguments for the command.</param>
        /// <param name="result">The `RunCommand` result.</param>
        public RunCommandException(string fileName, string args, RunCommandResult result)
            : base($"Command `{fileName} {args}` failed with: {result.ExitCode}\nOut: {result.StdOut}\nErr: {result.StdErr}")
        {
            this.FileName = fileName;
            this.Arguments = args;
            this.Result = result;
        }

        /// <summary>
        /// Gets or initializes the file name.
        /// </summary>
        public string FileName { get; private init; }

        /// <summary>
        /// Gets or initializes the arguments.
        /// </summary>
        public string Arguments { get; private init; }

        /// <summary>
        /// Gets or initializes the result object.
        /// </summary>
        public RunCommandResult Result { get; private init; }
    }
}
