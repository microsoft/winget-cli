// -----------------------------------------------------------------------------
// <copyright file="AddPinCmdlet.cs" company="Microsoft Corporation">
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
    /// Adds a pin for a package. Mirrors the behavior of <c>winget pin add</c>.
    /// </summary>
    [Cmdlet(
        VerbsCommon.Add,
        Constants.WinGetNouns.Pin,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [OutputType(typeof(PSPinResult))]
    public sealed class AddPinCmdlet : PackageCmdlet
    {
        private PinPackageCommand command = null;

        /// <summary>
        /// Gets or sets the pin type. Defaults to <see cref="PSPackagePinType.Pinning"/>.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSPackagePinType PinType { get; set; } = PSPackagePinType.Pinning;

        /// <summary>
        /// Gets or sets the gated version range. Required when <see cref="PinType"/> is Gating.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string GatedVersion { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to pin the installed version of the package.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter PinInstalledPackage { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to force the pin even if an existing pin is present.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Force { get; set; }

        /// <summary>
        /// Gets or sets an optional note to attach to the pin.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Note { get; set; }

        /// <summary>
        /// Adds a pin for the specified package.
        /// </summary>
        protected override void ProcessRecord()
        {
            this.command = new PinPackageCommand(
                this,
                this.PSCatalogPackage,
                this.Id,
                this.Name,
                this.Moniker,
                this.Source,
                this.Query);
            this.command.Add(
                this.MatchOption.ToString(),
                this.PinType.ToString(),
                this.GatedVersion,
                this.PinInstalledPackage.ToBool(),
                this.Force.ToBool(),
                this.Note);
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
