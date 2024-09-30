// -----------------------------------------------------------------------------
// <copyright file="ExportPackageCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;
    using Microsoft.WinGet.Client.Engine.PSObjects;

    /// <summary>
    /// Downloads a package installer from the pipeline or from a configured source.
    /// </summary>
    [Cmdlet(
        VerbsData.Export,
        Constants.WinGetNouns.Package,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [Alias("ewgp")]
    [OutputType(typeof(PSDownloadResult))]
    public sealed class ExportPackageCmdlet : InstallerSelectionCmdlet
    {
        private DownloadCommand command = null;

        /// <summary>
        /// Gets or sets the directory where the installer will be downloaded to.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string DownloadDirectory { get; set; }

        /// <summary>
        /// Installs a package from the pipeline or from a configured source.
        /// </summary>
        protected override void ProcessRecord()
        {
            this.command = new DownloadCommand(
                        this,
                        this.PSCatalogPackage,
                        this.Version,
                        this.Id,
                        this.Name,
                        this.Moniker,
                        this.Source,
                        this.Query,
                        this.AllowHashMismatch.ToBool(),
                        this.SkipDependencies.ToBool(),
                        this.Locale);
            this.command.Download(this.DownloadDirectory, this.MatchOption.ToString(), this.Scope.ToString(), this.Architecture.ToString(), this.InstallerType.ToString());
        }

        /// <summary>
        /// Interrupts currently running code within the command.
        /// </summary>
        protected override void StopProcessing()
        {
            if (this.command != null)
            {
                this.command.Cancel();
            }
        }
    }
}
