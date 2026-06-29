// -----------------------------------------------------------------------------
// <copyright file="GetPinCmdlet.cs" company="Microsoft Corporation">
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
    /// Gets package pins. If no filter is provided, returns all pins.
    /// </summary>
    [Cmdlet(
        VerbsCommon.Get,
        Constants.WinGetNouns.Pin,
        DefaultParameterSetName = Constants.AllSet)]
    [OutputType(typeof(PSPackagePin))]
    public sealed class GetPinCmdlet : FinderCmdlet
    {
        private PinPackageCommand command = null;

        /// <summary>
        /// Gets or sets the package to retrieve pins for.
        /// </summary>
        [Alias("InputObject")]
        [ValidateNotNull]
        [Parameter(
            ParameterSetName = Constants.GivenSet,
            Position = 0,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public PSCatalogPackage PSCatalogPackage { get; set; } = null;

        /// <summary>
        /// Retrieves pins based on the specified parameters.
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

            if (this.ParameterSetName == Constants.AllSet)
            {
                this.command.GetAll();
            }
            else
            {
                this.command.Get(this.MatchOption.ToString());
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
