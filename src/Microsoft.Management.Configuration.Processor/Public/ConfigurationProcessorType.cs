// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorType.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    /// <summary>
    /// Configuration processor runspace type.
    /// </summary>
    public enum ConfigurationProcessorType
    {
        /// <summary>
        /// Uses default runspace. Requires to be running in PowerShell. Uses current runspace.
        /// </summary>
        Default,

        /// <summary>
        /// Creates a new runspace in a hosted environment.
        /// </summary>
        Hosted,
    }
}
