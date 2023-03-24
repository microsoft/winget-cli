// -----------------------------------------------------------------------------
// <copyright file="SetUpFixture.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using static AppInstallerCLIE2ETests.TestCommon;

    /// <summary>
    /// An exception that occurred when running a command.
    /// </summary>
    internal class RunCommandException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="RunCommandException"/> class.
        /// </summary>
        /// <param name="result">The `RunCommand` result.</param>
        public RunCommandException(RunCommandResult result)
            : base($"Command failed with: {result.ExitCode}")
        {
            this.Result = result;
        }

        /// <summary>
        /// Gets or initializes the result object.
        /// </summary>
        public RunCommandResult Result { get; private init; }

        /// <summary>
        /// Creates a string representation of this object.
        /// </summary>
        /// <returns>A string representation of this object.</returns>
        public override string ToString()
        {
            return $"{this.Message}\nOut: {this.Result.StdOut}\nErr: {this.Result.StdErr}\n{this.StackTrace}";
        }
    }
}
