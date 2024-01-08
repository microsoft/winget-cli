// -----------------------------------------------------------------------------
// <copyright file="InstallerNestedInstallerFile.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    /// <summary>
    /// Installer nested installer file.
    /// </summary>
    public class InstallerNestedInstallerFile
    {
        /// <summary>
        /// Gets or sets relative file part.
        /// </summary>
        public string RelativeFilePath { get; set; }

        /// <summary>
        /// Gets or sets portable command alias.
        /// </summary>
        public string PortableCommandAlias { get; set; }
    }
}
