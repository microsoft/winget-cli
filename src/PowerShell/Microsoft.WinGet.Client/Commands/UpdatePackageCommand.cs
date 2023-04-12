// -----------------------------------------------------------------------------
// <copyright file="UpdatePackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.PSObjects;

    /// <summary>
    /// This commands updates a package from the pipeline or from the local system.
    /// </summary>
    [Cmdlet(
        VerbsData.Update,
        Constants.WinGetNouns.Package,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [OutputType(typeof(PSInstallResult))]
    public sealed class UpdatePackageCommand : InstallCommand
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
            // todo create installer command and call update.
        }
    }
}
