// -----------------------------------------------------------------------------
// <copyright file="InstallPackageCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Microsoft.WinGet.Client.PSObjects;

    /// <summary>
    /// Installs a package from the pipeline or from a configured source.
    /// </summary>
    [Cmdlet(
        VerbsLifecycle.Install,
        Constants.WinGetNouns.Package,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [OutputType(typeof(PSInstallResult))]
    public sealed class InstallPackageCmdlet : InstallCmdlet
    {
        /// <summary>
        /// Gets or sets the scope to install the application under.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSPackageInstallScope Scope { get; set; } = PSPackageInstallScope.Any;

        /// <summary>
        /// Gets or sets the architecture of the application to be installed.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSProcessorArchitecture Architecture { get; set; } = PSProcessorArchitecture.Default;

        /// <summary>
        /// Installs a package from the pipeline or from a configured source.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new InstallerPackageCommand(
                this,
                this.Mode.ToString(),
                this.Override,
                this.Custom,
                this.Location,
                this.AllowHashMismatch.ToBool(),
                this.Force.ToBool(),
                this.Header,
                this.PSCatalogPackage,
                this.Version,
                this.Log,
                this.Id,
                this.Name,
                this.Moniker,
                this.Source,
                this.Query,
                this.MatchOption.ToString());
            command.Install(this.Scope.ToString(), this.Architecture.ToString());
        }
    }
}
