// -----------------------------------------------------------------------------
// <copyright file="InstallCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands.Common
{
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Helpers;

    /// <summary>
    /// This is the base class for all commands that parse a <see cref="FindPackagesOptions" /> result
    /// from the provided parameters i.e., the "install" and "upgrade" commands.
    /// </summary>
    public abstract class InstallCommand : PackageCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InstallCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        internal InstallCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
        }

        /// <summary>
        /// Gets or sets a value indicating whether to skip the installer hash validation check.
        /// </summary>
        protected bool AllowHashMismatch { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to skip dependencies.
        /// </summary>
        protected bool SkipDependencies { get; set; }

        /// <summary>
        /// Gets or sets the override arguments to be passed on to the installer.
        /// </summary>
        protected string? Override { get; set; }

        /// <summary>
        /// Gets or sets the arguments to be passed on to the installer in addition to the defaults.
        /// </summary>
        protected string? Custom { get; set; }

        /// <summary>
        /// Gets or sets the installation location.
        /// </summary>
        protected string? Location { get; set; }

        /// <summary>
        /// Gets or sets the path to the logging file.
        /// </summary>
        protected string? Log { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to continue upon non security related failures.
        /// </summary>
        protected bool Force { get; set; }

        /// <summary>
        /// Gets or sets the optional HTTP Header to pass on to the REST Source.
        /// </summary>
        protected string? Header { get; set; }

        /// <summary>
        /// Gets the install options from the configured parameters.
        /// DO NOT pass PackageInstallMode WinRT enum type in this method.
        /// That will cause the type to attempt to be loaded in the construction
        /// of this method and throw a different exception for Windows PowerShell.
        /// </summary>
        /// <param name="version">The <see cref="PackageVersionId" /> to install.</param>
        /// <param name="mode">Package install mode as string.</param>
        /// <returns>An <see cref="InstallOptions" /> instance.</returns>
        internal virtual InstallOptions GetInstallOptions(PackageVersionId? version, string mode)
        {
            InstallOptions options = ManagementDeploymentFactory.Instance.CreateInstallOptions();
            options.AllowHashMismatch = this.AllowHashMismatch;
            options.SkipDependencies = this.SkipDependencies;
            options.Force = this.Force;
            options.PackageInstallMode = PSEnumHelpers.ToPackageInstallMode(mode);
            if (version != null)
            {
                options.PackageVersionId = version;
            }

            if (this.Log != null)
            {
                options.LogOutputPath = this.Log;
            }

            if (this.Override != null)
            {
                options.ReplacementInstallerArguments = this.Override;
            }

            // Since these arguments are appended to the installer at runtime, it doesn't make sense to append them if they are whitespace
            if (!string.IsNullOrWhiteSpace(this.Custom))
            {
                options.AdditionalInstallerArguments = this.Custom;
            }

            if (this.Location != null)
            {
                options.PreferredInstallLocation = this.Location;
            }

            if (this.Header != null)
            {
                options.AdditionalPackageCatalogArguments = this.Header;
            }

            return options;
        }
    }
}
