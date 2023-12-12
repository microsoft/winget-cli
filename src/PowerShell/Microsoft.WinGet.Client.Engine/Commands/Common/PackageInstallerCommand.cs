// -----------------------------------------------------------------------------
// <copyright file="PackageInstallerCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands.Common
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Engine.PSObjects;

    /// <summary>
    /// This is the base class for all commands that distinguish installers for a given package.
    /// Contains shared arguments for the installer, update, and download command.
    /// </summary>
    public abstract class PackageInstallerCommand : PackageCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PackageInstallerCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        internal PackageInstallerCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
        }

        /// <summary>
        /// Gets or sets a value indicating whether to skip the installer hash validation check.
        /// </summary>
        protected bool AllowHashMismatch { get; set; }

        /// <summary>
        /// Gets or sets the installer locale.
        /// </summary>
        protected string? Locale { get; set; }

        /// <summary>
        /// Gets or sets the installer architecture.
        /// </summary>
        protected PSProcessorArchitecture Architecture { get; set; }

        /// <summary>
        /// Gets or sets the installer scope.
        /// </summary>
        protected PSPackageInstallScope Scope { get; set; }

        /// <summary>
        /// Gets or sets the installer type.
        /// </summary>
        protected PSPackageInstallerType InstallerType { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to skip dependencies.
        /// </summary>
        protected bool SkipDependencies { get; set; }
    }
}
