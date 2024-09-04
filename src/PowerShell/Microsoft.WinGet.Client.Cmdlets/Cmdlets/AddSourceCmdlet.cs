// -----------------------------------------------------------------------------
// <copyright file="AddSourceCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Cmdlets.Cmdlets
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Cmdlets.PSObjects;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.Commands;

    /// <summary>
    /// Adds a source. Requires admin.
    /// </summary>
    [Cmdlet(VerbsCommon.Add, Constants.WinGetNouns.Source)]
    public sealed class AddSourceCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the name of the source to add.
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the argument of the source to add.
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public string Argument { get; set; }

        /// <summary>
        /// Gets or sets the type of the source to add.
        /// </summary>
        [Parameter(
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public string Type { get; set; }

        /// <summary>
        /// Gets or sets the trust level of the source to add.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSSourceTrustLevel TrustLevel { get; set; } = PSSourceTrustLevel.Default;

        /// <summary>
        /// Gets or sets a value indicating whether the source to add is explicit.
        /// </summary>
        ///
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Explicit { get; set; }

        /// <summary>
        /// Adds source.
        /// </summary>
        protected override void ProcessRecord()
        {
            var command = new CliCommand(this);
            command.AddSource(this.Name, this.Argument, this.Type, this.ConvertPSSourceTrustLevelToString(this.TrustLevel), this.Explicit.ToBool());
        }

        private string ConvertPSSourceTrustLevelToString(PSSourceTrustLevel trustLevel)
        {
            string trustLevelString = string.Empty;
            switch (trustLevel)
            {
                case PSSourceTrustLevel.None:
                    trustLevelString = "None";
                    break;
                case PSSourceTrustLevel.Trusted:
                    trustLevelString = "Trusted";
                    break;
            }

            return trustLevelString;
        }
    }
}
