// -----------------------------------------------------------------------------
// <copyright file="InstallPackageCmdlet.cs" company="Microsoft Corporation">
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
    /// Installs a package from the pipeline or from a configured source.
    /// </summary>
    [Cmdlet(
        VerbsLifecycle.Install,
        Constants.WinGetNouns.Package,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [Alias("iswgp")]
    [OutputType(typeof(PSInstallResult))]
    public sealed class InstallPackageCmdlet : InstallCmdlet
    {
        private InstallerPackageCommand command = null;

        /// <summary>
        /// Installs a package from the pipeline or from a configured source.
        /// </summary>
        protected override void ProcessRecord()
        {
            this.command = new InstallerPackageCommand(
                        this,
                        this.Override,
                        this.Custom,
                        this.Location,
                        this.AllowHashMismatch.ToBool(),
                        this.Force.ToBool(),
                        this.Header,
                        this.PSCatalogPackage,
                        this.Version,
                        this.Log,
                        this.Id,
                        this.Name,
                        this.Moniker,
                        this.Source,
                        this.Query,
                        this.SkipDependencies.ToBool());

            this.command.Install(this.MatchOption.ToString(), this.Scope.ToString(), this.Architecture.ToString(), this.Mode.ToString(), this.InstallerType.ToString());
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
