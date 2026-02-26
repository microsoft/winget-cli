// -----------------------------------------------------------------------------
// <copyright file="ManifestLocalization.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.Preview
{
    /// <summary>
    /// Class that contains the elements that represent what a manifest localization element can contain in the manifest file.
    /// </summary>
    public class ManifestLocalization
    {
        /// <summary>
        /// Gets or sets the localization type of manifest. Required element.
        /// </summary>
        public string Language { get; set; }

        /// <summary>
        /// Gets or sets the description of the manifest.
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Gets or sets the homepage related to the package.
        /// </summary>
        public string Homepage { get; set; }

        /// <summary>
        /// Gets or sets the license URL.
        /// </summary>
        public string LicenseUrl { get; set; }
    }
}
