// -----------------------------------------------------------------------------
// <copyright file="ProcessExecutionEnvironmentVariableValueType.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    /// <summary>
    /// The environment variable value type.
    /// </summary>
    internal enum ProcessExecutionEnvironmentVariableValueType
    {
        /// <summary>
        /// Prepend.
        /// </summary>
        Prepend,

        /// <summary>
        /// Append.
        /// </summary>
        Append,

        /// <summary>
        /// Override.
        /// </summary>
        Override,
    }
}
