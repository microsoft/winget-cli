// -----------------------------------------------------------------------------
// <copyright file="PowerShellConfigurationProcessorLocation.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    /// <summary>
    /// The location where modules are going to be installed.
    /// </summary>
    public enum PowerShellConfigurationProcessorLocation
    {
        /// <summary>
        /// Current user path.
        /// </summary>
        CurrentUser = 0,

        /// <summary>
        /// AllUsers path. Requires admin.
        /// </summary>
        AllUsers = 1,

        /// <summary>
        /// The winget location %LOCALAPPDATA%\Microsoft\WinGet\Configuration\Modules.
        /// </summary>
        WinGetModulePath = 2,

        /// <summary>
        /// Custom path.
        /// </summary>
        Custom = 3,

        /// <summary>
        /// Default.
        /// </summary>
        Default = WinGetModulePath,
    }
}
