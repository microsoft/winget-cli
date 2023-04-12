// -----------------------------------------------------------------------------
// <copyright file="UninstallPackageCommand.cs" company="Microsoft Corporation">
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
    /// Uninstalls a package from the local system.
    /// </summary>
    [Cmdlet(
        VerbsLifecycle.Uninstall,
        Constants.WinGetNouns.Package,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [OutputType(typeof(PSUninstallResult))]
    public sealed class UninstallPackageCommand : PackageCommand
    {
        /// <summary>
        /// Gets or sets the desired mode for the uninstallation process.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSPackageUninstallMode Mode { get; set; } = PSPackageUninstallMode.Default;

        /// <summary>
        /// Gets or sets a value indicating whether to continue upon non security related failures.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Force { get; set; }

        /// <summary>
        /// Uninstalls a package from the local system.
        /// </summary>
        protected override void ProcessRecord()
        {
            // tODO create command call uninstall.
        }
    }
}
