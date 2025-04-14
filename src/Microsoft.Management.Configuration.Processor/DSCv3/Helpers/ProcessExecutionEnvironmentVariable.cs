// -----------------------------------------------------------------------------
// <copyright file="ProcessExecutionEnvironmentVariable.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    /// <summary>
    /// Contains custom environment variable info for ProcessExecution.
    /// </summary>
    internal class ProcessExecutionEnvironmentVariable
    {
        /// <summary>
        /// Gets the name of the environment variable.
        /// </summary>
        required public string Name { get; init; }

        /// <summary>
        /// Gets the value of the environment variable.
        /// </summary>
        required public string Value { get; init; }

        /// <summary>
        /// Gets the value type of the environment variable.
        /// </summary>
        public ProcessExecutionEnvironmentVariableValueType ValueType { get; init; } = ProcessExecutionEnvironmentVariableValueType.Override;

        /// <summary>
        /// Gets the separator of the environment variable if value type is prepend or append.
        /// </summary>
        public string Separator { get; init; } = ";";
    }
}
