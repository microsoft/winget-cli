// -----------------------------------------------------------------------------
// <copyright file="GetPackageInstallerCmdlet.cs" company="Microsoft Corporation">
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
    /// Downloads a package installer from the pipeline or from a configured source.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, Constants.WinGetNouns.PackageInstaller)]
    [OutputType(typeof(PSDownloadResult))]
    public sealed class GetPackageInstallerCmdlet : PackageCmdlet
    {
        private DownloadCommand command = null;

        /// <summary>
        /// Installs a package from the pipeline or from a configured source.
        /// </summary>
        protected override void ProcessRecord()
        {
            // DownloadDirectory
            // SKip Dependencies
            // Installer-Type (Do this for update and install)
            // Architecture
            // Locale
            this.command = new DownloadCommand(
                        this,
                        this.PSCatalogPackage,
                        this.Version,
                        this.Log,
                        this.Id,
                        this.Name,
                        this.Moniker,
                        this.Source,
                        this.Query);
            this.command.Download(this.MatchOption.ToString());
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
