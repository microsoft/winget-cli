// -----------------------------------------------------------------------------
// <copyright file="ManifestInstallerFile.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
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
