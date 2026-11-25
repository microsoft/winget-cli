// -----------------------------------------------------------------------------
// <copyright file="PowerShellConfigurationProcessorType.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    /// <summary>
    /// Configuration processor runspace type.
    /// </summary>
    public enum PowerShellConfigurationProcessorType
    {
        /// <summary>
        /// Uses default runspace. Requires to be running in PowerShell. Uses current runspace.
        /// Module locations use PowerShell scopes without reading WinGet settings.
        /// </summary>
        Default,

        /// <summary>
        /// Creates a new runspace in a hosted environment.
        /// Module locations use WinGet settings and native runtime defaults.
        /// </summary>
        Hosted,
    }
}
