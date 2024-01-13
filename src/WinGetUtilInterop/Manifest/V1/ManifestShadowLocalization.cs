// -----------------------------------------------------------------------------
// <copyright file="ManifestShadowLocalization.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System.Collections.Generic;

    /// <summary>
    /// Localization properties for shadow manifest.
    /// </summary>
    public class ManifestShadowLocalization
    {
        /// <summary>
        /// Gets or sets the locale.
        /// </summary>
        public string PackageLocale { get; set; }

        /// <summary>
        /// Gets or sets the manifest icons information.
        /// </summary>
        public List<ManifestIcon> Icons { get; set; }

        /// <summary>
        /// Returns a List of strings containing the URIs contained within this localization.
        /// </summary>
        /// <returns>List of strings.</returns>
        public List<string> GetURIs()
        {
            List<string> uris = new List<string>();

            if (this.Icons != null)
            {
                foreach (var icon in this.Icons)
                {
                    if (!string.IsNullOrEmpty(icon.IconUrl))
                    {
                        uris.Add(icon.IconUrl);
                    }
                }
            }

            return uris;
        }
    }
}
