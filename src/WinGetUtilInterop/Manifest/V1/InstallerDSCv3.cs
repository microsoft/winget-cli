// -----------------------------------------------------------------------------
// <copyright file="InstallerDSCv3.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System.Collections.Generic;

    /// <summary>
    /// DSCv3 resource info.
    /// </summary>
    public class InstallerDSCv3
    {
        /// <summary>
        /// Gets or sets the list of DSCv3 resources.
        /// </summary>
        public List<InstallerDSCv3Resource> Resources { get; set; }
    }
}
