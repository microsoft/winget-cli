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
    /// <remarks>
    /// Pin type is determined by which parameters are provided, matching <c>winget pin add</c> behavior:
    /// <list type="bullet">
    ///   <item><description><see cref="GatedVersion"/> present → Gating pin</description></item>
    ///   <item><description><see cref="Blocking"/> present → Blocking pin</description></item>
    ///   <item><description>Neither → Pinning pin (default)</description></item>
    /// </list>
    /// </remarks>
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
        /// Gets or sets a value indicating whether to add a blocking pin, which prevents all upgrades.
        /// Cannot be combined with <see cref="GatedVersion"/>.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Blocking { get; set; }

        /// <summary>
        /// Gets or sets the gated version range, which creates a gating pin that limits upgrades to
        /// versions satisfying this range (e.g., <c>&lt;7.5</c>, <c>&gt;=7.0,&lt;8.0</c>).
        /// Cannot be combined with <see cref="Blocking"/>.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        [ValidateNotNullOrEmpty]
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
            if (this.Blocking && !string.IsNullOrEmpty(this.GatedVersion))
            {
                this.ThrowTerminatingError(new ErrorRecord(
                    new System.ArgumentException("Cannot specify both -Blocking and -GatedVersion. Use -GatedVersion for a gating pin or -Blocking for a blocking pin."),
                    "ConflictingPinTypeParameters",
                    ErrorCategory.InvalidArgument,
                    null));
            }

            PSPackagePinType pinType = !string.IsNullOrEmpty(this.GatedVersion) ? PSPackagePinType.Gating
                           : this.Blocking ? PSPackagePinType.Blocking
                           : PSPackagePinType.Pinning;

            string target = this.PSCatalogPackage?.Id
                ?? this.Id
                ?? this.Name
                ?? (this.Query != null ? string.Join(" ", this.Query) : null)
                ?? "package";

            if (!this.ShouldProcess(target))
            {
                return;
            }

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
                pinType.ToString(),
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
