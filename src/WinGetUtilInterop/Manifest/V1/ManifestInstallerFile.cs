// -----------------------------------------------------------------------
// <copyright file="ManifestInstallationMetadata.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
// -----------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System;
    using System.Collections.Generic;
    using System.Text;

    /// <summary>
    /// Represents an installer file.
    /// </summary>
    public class ManifestInstallerFile
    {
        /// <summary>
        /// Gets or sets relative file path.
        /// </summary>
        public string RelativeFilePath { get; set; }

        /// <summary>
        /// Gets or sets file sha256.
        /// </summary>
        public string FileSha256 { get; set; }

        /// <summary>
        /// Gets or sets file type.
        /// </summary>
        public string FileType { get; set; }

        /// <summary>
        /// Gets or sets invocation parameter.
        /// </summary>
        public string InvocationParameter { get; set; }

        /// <summary>
        /// Gets or sets display name.
        /// </summary>
        public string DisplayName { get; set; }
    }
}
