// -----------------------------------------------------------------------------
// <copyright file="ManifestIcon.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    /// <summary>
    /// Icon information.
    /// </summary>
    public class ManifestIcon
    {
        /// <summary>
        /// Gets or sets the icon url.
        /// </summary>
        public string IconUrl { get; set; }

        /// <summary>
        /// Gets or sets the icon file type.
        /// </summary>
        public string IconFileType { get; set; }

        /// <summary>
        /// Gets or sets the icon resolution.
        /// </summary>
        public string IconResolution { get; set; }

        /// <summary>
        /// Gets or sets the icon theme.
        /// </summary>
        public string IconTheme { get; set; }

        /// <summary>
        /// Gets or sets the icon sha256.
        /// </summary>
        public string IconSha256 { get; set; }
    }
}
