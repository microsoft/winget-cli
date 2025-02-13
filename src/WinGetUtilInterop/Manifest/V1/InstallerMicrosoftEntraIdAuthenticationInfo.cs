// -----------------------------------------------------------------------------
// <copyright file="InstallerMicrosoftEntraIdAuthenticationInfo.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    /// <summary>
    /// Microsoft Entra Id authentication info.
    /// </summary>
    public class InstallerMicrosoftEntraIdAuthenticationInfo
    {
        /// <summary>
        /// Gets or sets the authentication resource.
        /// </summary>
        public string Resource { get; set; }

        /// <summary>
        /// Gets or sets the authentication scope.
        /// </summary>
        public string Scope { get; set; }
    }
}
