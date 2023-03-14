// -----------------------------------------------------------------------------
// <copyright file="InstallPackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Properties;
    using Windows.System;

    /// <summary>
    /// Installs a package from the pipeline or from a configured source.
    /// </summary>
    [Cmdlet(
        VerbsLifecycle.Install,
        Constants.WinGetNouns.Package,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [OutputType(typeof(PSObjects.InstallResult))]
    public sealed class InstallPackageCommand : BaseInstallCommand
    {
        private ProcessorArchitecture architecture;
        private bool architectureGiven;

        /// <summary>
        /// Gets or sets the scope to install the application under.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PackageInstallScope Scope { get; set; }

        /// <summary>
        /// Gets or sets the architecture of the application to be installed.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public ProcessorArchitecture Architecture
        {
            get => this.architecture;
            set
            {
                this.architectureGiven = true;
                this.architecture = value;
            }
        }

        /// <summary>
        /// Prepares the command to install a package.
        /// </summary>
        protected override void BeginProcessing()
        {
            base.BeginProcessing();
            this.architectureGiven = false;
        }

        /// <summary>
        /// Installs a package from the pipeline or from a configured source.
        /// </summary>
        protected override void ProcessRecord()
        {
            base.ProcessRecord();
            this.GetPackageAndExecute(CompositeSearchBehavior.RemotePackagesFromRemoteCatalogs, (package, version) =>
            {
                InstallOptions options = this.GetInstallOptions(version);
                InstallResult result = this.InstallPackage(package, options);
                this.WriteObject(new PSObjects.InstallResult(result));
            });
        }

        /// <inheritdoc />
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "Targets Windows 10.0.17763.0")]
        protected override InstallOptions GetInstallOptions(PackageVersionId version)
        {
            InstallOptions options = base.GetInstallOptions(version);
            if (this.architectureGiven)
            {
                options.AllowedArchitectures.Clear();
                options.AllowedArchitectures.Add(this.architecture);
            }

            options.PackageInstallScope = this.Scope;
            return options;
        }

        private InstallResult InstallPackage(
            CatalogPackage package,
            InstallOptions options)
        {
            var operation = PackageManager.Value.InstallPackageAsync(package, options);
            return this.RegisterCallbacksAndWait(operation, string.Format(
                Resources.ProgressRecordActivityInstalling,
                package.Name));
        }
    }
}
