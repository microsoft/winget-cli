// -----------------------------------------------------------------------------
// <copyright file="InstallerArpEntry.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    /// <summary>
    /// Class that contains installer arp entry.
    /// </summary>
    public class InstallerArpEntry
    {
        /// <summary>
        /// Gets or sets the display name.
        /// </summary>
        public string DisplayName { get; set; }

        /// <summary>
        /// Gets or sets the publisher.
        /// </summary>
        public string Publisher { get; set; }

        /// <summary>
        /// Gets or sets the display version.
        /// </summary>
        public string DisplayVersion { get; set; }

        /// <summary>
        /// Gets or sets the ProductCode.
        /// </summary>
        public string ProductCode { get; set; }

        /// <summary>
        /// Gets or sets the UpgradeCode.
        /// </summary>
        public string UpgradeCode { get; set; }

        /// <summary>
        /// Gets or sets the installer type.
        /// </summary>
        public string InstallerType { get; set; }
    }
}