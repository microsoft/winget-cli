// -----------------------------------------------------------------------------
// <copyright file="PSConfigurationTestResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    /// <summary>
    /// Must match ConfigurationTestResult.
    /// </summary>
    public enum PSConfigurationTestResult
    {
        /// <summary>
        /// The result is unknown.
        /// </summary>
        Unknown,

        /// <summary>
        /// The system is in the state described by the configuration.
        /// </summary>
        Positive,

        /// <summary>
        /// The system is not in the state described by the configuration.
        /// </summary>
        Negative,

        /// <summary>
        /// Running the test failed.
        /// </summary>
        Failed,

        /// <summary>
        /// The test was not run because it was not applicable.
        /// </summary>
        NotRun,
    }
}
