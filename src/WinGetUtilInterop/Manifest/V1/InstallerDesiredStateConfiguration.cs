// -----------------------------------------------------------------------------
// <copyright file="InstallerDesiredStateConfiguration.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System.Collections.Generic;

    /// <summary>
    /// Desired state configuration info for an installer.
    /// </summary>
    public class InstallerDesiredStateConfiguration
    {
        /// <summary>
        /// Gets or sets the list of PowerShell DSC module items.
        /// </summary>
        public List<InstallerDSCPowerShellModule> PowerShell { get; set; }

        /// <summary>
        /// Gets or sets the DSCv3 resource info.
        /// </summary>
        public InstallerDSCv3 DSCv3 { get; set; }
    }
}
