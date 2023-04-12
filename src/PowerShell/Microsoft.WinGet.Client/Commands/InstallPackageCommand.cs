// -----------------------------------------------------------------------------
// <copyright file="InstallPackageCommand.cs" company="Microsoft Corporation">
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
    /// Installs a package from the pipeline or from a configured source.
    /// </summary>
    [Cmdlet(
        VerbsLifecycle.Install,
        Constants.WinGetNouns.Package,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [OutputType(typeof(PSInstallResult))]
    public sealed class InstallPackageCommand : InstallCommand
    {
        /// <summary>
        /// Gets or sets the scope to install the application under.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSPackageInstallScope Scope { get; set; } = PSPackageInstallScope.Any;

        /// <summary>
        /// Gets or sets the architecture of the application to be installed.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSProcessorArchitecture Architecture { get; set; } = PSProcessorArchitecture.None;

        /// <summary>
        /// Installs a package from the pipeline or from a configured source.
        /// </summary>
        protected override void ProcessRecord()
        {
            // TODO call installercommand and install
        }
    }
}
