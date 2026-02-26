// -----------------------------------------------------------------------------
// <copyright file="InstallerExpectedReturnCode.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    /// <summary>
    /// Class that contains installer expected return code.
    /// </summary>
    public class InstallerExpectedReturnCode
    {
        /// <summary>
        /// Gets or sets the installer return code.
        /// </summary>
        public long InstallerReturnCode { get; set; }

        /// <summary>
        /// Gets or sets the corresponding response category.
        /// </summary>
        public string ReturnResponse { get; set; }

        /// <summary>
        /// Gets or sets the corresponding response url.
        /// </summary>
        public string ReturnResponseUrl { get; set; }
    }
}
