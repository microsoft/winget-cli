// -----------------------------------------------------------------------------
// <copyright file="RemovePinCmdlet.cs" company="Microsoft Corporation">
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
    /// Removes a package pin. Mirrors the behavior of <c>winget pin remove</c>.
    /// Accepts a <see cref="PSPackagePin"/> from the pipeline (e.g., from <c>Get-WinGetPin</c>),
    /// a <see cref="PSCatalogPackage"/> from the pipeline, or package search criteria.
    /// </summary>
    [Cmdlet(
        VerbsCommon.Remove,
        Constants.WinGetNouns.Pin,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [OutputType(typeof(PSPinResult))]
    public sealed class RemovePinCmdlet : PackageCmdlet
    {
        private PinPackageCommand command = null;

        /// <summary>
        /// Gets or sets the pin object to remove. Accepts pipeline input from <c>Get-WinGetPin</c>.
        /// </summary>
        [ValidateNotNull]
        [Parameter(
            ParameterSetName = Constants.PinSet,
            Position = 0,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public PSPackagePin PSPackagePin { get; set; } = null;

        /// <summary>
        /// Removes the pin for the specified package.
        /// </summary>
        protected override void ProcessRecord()
        {
            PSCatalogPackage catalogPackage = this.PSCatalogPackage;
            string id = this.Id;

            if (this.ParameterSetName == Constants.PinSet)
            {
                id = this.PSPackagePin.PackageId;
            }

            this.command = new PinPackageCommand(
                this,
                catalogPackage,
                id,
                this.Name,
                this.Moniker,
                this.Source,
                this.Query);
            this.command.Remove(this.MatchOption.ToString());
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
