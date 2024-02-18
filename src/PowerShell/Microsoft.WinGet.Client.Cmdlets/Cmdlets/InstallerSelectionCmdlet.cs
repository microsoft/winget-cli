// -----------------------------------------------------------------------------
// <copyright file="InstallerSelectionCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.PSObjects;

    /// <summary>
    /// This is the base class for all commands that select an installer from a given package.
    /// Contains shared arguments for the install, update, and download commands.
    /// </summary>
    public abstract class InstallerSelectionCmdlet : PackageCmdlet
    {
        /// <summary>
        /// Gets or sets a value indicating whether to skip the installer hash validation check.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter AllowHashMismatch { get; set; }

        /// <summary>
        /// Gets or sets the architecture of the installer to be downloaded.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSProcessorArchitecture Architecture { get; set; } = PSProcessorArchitecture.Default;

        /// <summary>
        /// Gets or sets the installer type to be downloaded.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSPackageInstallerType InstallerType { get; set; } = PSPackageInstallerType.Default;

        /// <summary>
        /// Gets or sets the locale of the installer to be downloaded.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Locale { get; set; }

        /// <summary>
        /// Gets or sets the scope of the installer to be downloaded.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSPackageInstallScope Scope { get; set; } = PSPackageInstallScope.Any;

        /// <summary>
        /// Gets or sets a value indicating whether skip dependencies.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter SkipDependencies { get; set; }
    }
}
