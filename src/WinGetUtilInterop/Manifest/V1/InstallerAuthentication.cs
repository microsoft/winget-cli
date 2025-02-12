// -----------------------------------------------------------------------------
// <copyright file="InstallerAuthentication.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    /// <summary>
    /// Installer authentication info.
    /// </summary>
    public class InstallerAuthentication
    {
        /// <summary>
        /// Gets or sets the authentication type.
        /// </summary>
        public string AuthenticationType { get; set; }

        /// <summary>
        /// Gets or sets the Microsoft Entra Id authentication info.
        /// </summary>
        public InstallerMicrosoftEntraIdAuthenticationInfo MicrosoftEntraIdAuthenticationInfo { get; set; }
    }
}
