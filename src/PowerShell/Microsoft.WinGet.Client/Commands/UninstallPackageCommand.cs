// -----------------------------------------------------------------------------
// <copyright file="UninstallPackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Helpers;
    using Microsoft.WinGet.Client.Properties;

    /// <summary>
    /// Uninstalls a package from the local system.
    /// </summary>
    [Cmdlet(
        VerbsLifecycle.Uninstall,
        Constants.WinGetNouns.Package,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [OutputType(typeof(PSObjects.UninstallResult))]
    public sealed class UninstallPackageCommand : BasePackageCommand
    {
        /// <summary>
        /// Gets or sets the desired mode for the uninstallation process.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PackageUninstallMode Mode { get; set; } = PackageUninstallMode.Default;

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
            this.GetPackageAndExecute(CompositeSearchBehavior.LocalCatalogs, (package, version) =>
            {
                UninstallOptions options = this.GetUninstallOptions(version);
                UninstallResult result = this.UninstallPackage(package, options);
                this.WriteObject(new PSObjects.UninstallResult(result));
            });
        }

        private UninstallOptions GetUninstallOptions(PackageVersionId version)
        {
            var options = ComObjectFactory.Value.CreateUninstallOptions();
            options.Force = this.Force.ToBool();
            if (this.Log != null)
            {
                options.LogOutputPath = this.Log;
            }

            options.PackageUninstallMode = this.Mode;
            if (version != null)
            {
                options.PackageVersionId = version;
            }

            return options;
        }

        private UninstallResult UninstallPackage(
            CatalogPackage package,
            UninstallOptions options)
        {
            string activity = string.Format(
                Resources.ProgressRecordActivityUninstalling,
                package.Name);

            var operation = PackageManager.Value.UninstallPackageAsync(package, options);
            WriteProgressAdapter adapter = new (this);
            operation.Progress = (context, progress) =>
            {
                adapter.WriteProgress(new ProgressRecord(1, activity, progress.State.ToString())
                {
                    RecordType = ProgressRecordType.Processing,
                });
            };
            operation.Completed = (context, status) =>
            {
                adapter.WriteProgress(new ProgressRecord(1, activity, status.ToString())
                {
                    RecordType = ProgressRecordType.Completed,
                });
                adapter.Completed = true;
            };
            Console.CancelKeyPress += (sender, e) =>
            {
                operation.Cancel();
            };
            adapter.Wait();
            return operation.GetResults();
        }
    }
}
