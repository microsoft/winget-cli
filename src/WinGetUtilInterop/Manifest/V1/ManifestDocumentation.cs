// -----------------------------------------------------------------------------
// <copyright file="ManifestDocumentation.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    /// <summary>
    /// Manifest documentation.
    /// </summary>
    public class ManifestDocumentation
    {
        /// <summary>
        /// Gets or sets the document label.
        /// </summary>
        public string DocumentLabel { get; set; }

        /// <summary>
        /// Gets or sets the document url.
        /// </summary>
        public string DocumentUrl { get; set; }
    }
}
