// -----------------------------------------------------------------------------
// <copyright file="InstallerDSCPowerShellModule.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System.Collections.Generic;

    /// <summary>
    /// PowerShell DSC module item.
    /// </summary>
    public class InstallerDSCPowerShellModule
    {
        /// <summary>
        /// Gets or sets the repository URL.
        /// </summary>
        public string RepositoryUrl { get; set; }

        /// <summary>
        /// Gets or sets the module name.
        /// </summary>
        public string ModuleName { get; set; }

        /// <summary>
        /// Gets or sets the list of resources contained in the module.
        /// </summary>
        public List<InstallerDSCPowerShellResource> Resources { get; set; }
    }
}
