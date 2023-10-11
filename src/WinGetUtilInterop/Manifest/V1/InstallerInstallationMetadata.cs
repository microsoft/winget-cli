// -----------------------------------------------------------------------
// <copyright file="InstallerInstallationMetadata.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
// -----------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System;
    using System.Collections.Generic;
    using System.Text;

    public class InstallerInstallationMetadata
    {
        /// <summary>
        /// Gets or sets the default install location.
        /// </summary>
        public string DefaultInstallLocation { get; set; }

        /// <summary>
        /// Gets or sets the manifest installer files.
        /// </summary>
        public List<ManifestInstallerFile> Files { get; set; }
    }
}
