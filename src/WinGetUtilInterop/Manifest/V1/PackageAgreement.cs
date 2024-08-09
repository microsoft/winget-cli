// -----------------------------------------------------------------------------
// <copyright file="PackageAgreement.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    /// <summary>
    /// Class that contains package agreement.
    /// </summary>
    public class PackageAgreement
    {
        /// <summary>
        /// Gets or sets the agreement label.
        /// </summary>
        public string AgreementLabel { get; set; }

        /// <summary>
        /// Gets or sets the agreement text.
        /// </summary>
        public string Agreement { get; set; }

        /// <summary>
        /// Gets or sets the agreement url.
        /// </summary>
        public string AgreementUrl { get; set; }
    }
}