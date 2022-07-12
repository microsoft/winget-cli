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
    using Microsoft.WinGet.Client.Helpers;

    /// <summary>
    /// Uninstalls a package from the local system.
    /// </summary>
    [Cmdlet(
        VerbsLifecycle.Uninstall,
        Constants.PackageNoun,
        DefaultParameterSetName = Constants.FoundSet,
        SupportsShouldProcess = true)]
    [OutputType(typeof(UninstallResult))]
    public sealed class UninstallPackageCommand : BaseLifecycleCommand
    {
        /// <summary>
        /// Gets or sets the desired mode for the uninstallation process.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PackageUninstallMode Mode { get; set; } = PackageUninstallMode.Default;

        /// <summary>
        /// Uninstalls a package from the local system.
        /// </summary>
        protected override void ProcessRecord()
        {
            this.GetPackageAndExecute(CompositeSearchBehavior.LocalCatalogs, (package, version) =>
            {
                var options = this.GetUninstallOptions(version);
                var results = this.UninstallPackage(package, options);
                this.WriteObject(results);
            });
        }

        private UninstallOptions GetUninstallOptions(PackageVersionId version)
        {
            var options = ComObjectFactory.Value.CreateUninstallOptions();

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
            string activity = $"Uninstalling '{package.Name}'";

            var operation = PackageManager.Value.UninstallPackageAsync(package, options);
            var adapter = new WriteProgressAdapter(this);

            operation.Progress = (context, progress) =>
            {
                var record = new ProgressRecord(1, activity, progress.State.ToString())
                {
                    RecordType = ProgressRecordType.Processing,
                };

                adapter.WriteProgress(record);
            };

            operation.Completed = (context, status) =>
            {
                var record = new ProgressRecord(1, activity, status.ToString())
                {
                    RecordType = ProgressRecordType.Completed,
                };

                adapter.WriteProgress(record);
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
