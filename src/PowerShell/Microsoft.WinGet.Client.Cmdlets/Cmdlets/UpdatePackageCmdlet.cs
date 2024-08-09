// -----------------------------------------------------------------------------
// <copyright file="UpdatePackageCmdlet.cs" company="Microsoft Corporation">
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

    /// <summary>
    /// This commands updates a package from the pipeline or from the local system.
    /// </summary>
    [Cmdlet(
        VerbsData.Update,
        Constants.WinGetNouns.Package,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [Alias("udwgp")]
    [OutputType(typeof(PSInstallResult))]
    public sealed class UpdatePackageCmdlet : InstallCmdlet
    {
        /// <summary>
        /// Gets or sets a value indicating whether updating to an unknown version is allowed.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter IncludeUnknown { get; set; }

        /// <summary>
        /// Updates a package from the pipeline or from the local system.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new InstallerPackageCommand(
                        this,
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
                        this.SkipDependencies);
            command.Update(this.IncludeUnknown.ToBool(), this.MatchOption.ToString(), this.Scope.ToString(), this.Architecture.ToString(), this.Mode.ToString(), this.InstallerType.ToString());
        }
    }
}
