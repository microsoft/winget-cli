// -----------------------------------------------------------------------------
// <copyright file="ResetPinCmdlet.cs" company="Microsoft Corporation">
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
    /// Resets all package pins, optionally scoped to a source. Mirrors the behavior of <c>winget pin reset</c>.
    /// </summary>
    [Cmdlet(
        VerbsCommon.Reset,
        Constants.WinGetNouns.Pin,
        SupportsShouldProcess = true)]
    [OutputType(typeof(PSPinResult))]
    public sealed class ResetPinCmdlet : PSCmdlet
    {
        private ResetPinCommand command = null;

        /// <summary>
        /// Gets or sets the source name to scope the reset. If not specified, all sources are reset.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Source { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to skip confirmation.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Force { get; set; }

        /// <summary>
        /// Resets pins.
        /// </summary>
        protected override void ProcessRecord()
        {
            string target = string.IsNullOrEmpty(this.Source) ? "All sources" : this.Source;
            if (this.Force || this.ShouldProcess(target))
            {
                this.command = new ResetPinCommand(this);
                this.command.Reset(this.Source);
            }
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
