// -----------------------------------------------------------------------------
// <copyright file="PowerShellConfigurationProcessorScope.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    /// <summary>
    /// The scope where modules are going to be installed.
    /// </summary>
    public enum PowerShellConfigurationProcessorScope
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
        /// Custom path.
        /// </summary>
        Custom = 2,
    }
}
