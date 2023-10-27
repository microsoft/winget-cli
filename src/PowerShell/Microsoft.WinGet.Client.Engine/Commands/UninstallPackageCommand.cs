// -----------------------------------------------------------------------------
// <copyright file="UninstallPackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Uninstalls a package from the local system.
    /// </summary>
    public sealed class UninstallPackageCommand : PackageCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="UninstallPackageCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">Caller cmdlet.</param>
        /// <param name="psCatalogPackage">PSCatalogPackage.</param>
        /// <param name="version">Version to install.</param>
        /// <param name="log">Logging file location.</param>
        /// <param name="id">Package identifier.</param>
        /// <param name="name">Name of package.</param>
        /// <param name="moniker">Moniker of package.</param>
        /// <param name="source">Source to search. If null, all are searched.</param>
        /// <param name="query">Match against any field of a package.</param>
        public UninstallPackageCommand(
            PSCmdlet psCmdlet,
            PSCatalogPackage psCatalogPackage,
            string version,
            string log,
            string id,
            string name,
            string moniker,
            string source,
            string[] query)
            : base(psCmdlet)
        {
            // PackageCommand.
            if (psCatalogPackage != null)
            {
                this.CatalogPackage = psCatalogPackage;
            }

            this.Version = version;
            this.Log = log;

            // FinderCommand
            this.Id = id;
            this.Name = name;
            this.Moniker = moniker;
            this.Source = source;
            this.Query = query;
        }

        /// <summary>
        /// Process uninstall package.
        /// </summary>
        /// <param name="psPackageUninstallMode">PSPackageUninstallMode.</param>
        /// <param name="psPackageFieldMatchOption">PSPackageFieldMatchOption.</param>
        /// <param name="force">Force.</param>
        public void Uninstall(
            string psPackageUninstallMode,
            string psPackageFieldMatchOption,
            bool force)
        {
            this.GetPackageAndExecute(
                CompositeSearchBehavior.LocalCatalogs,
                PSEnumHelpers.ToPackageFieldMatchOption(psPackageFieldMatchOption),
                (package, version) =>
                {
                    UninstallOptions options = this.GetUninstallOptions(version, PSEnumHelpers.ToPackageUninstallMode(psPackageUninstallMode), force);
                    UninstallResult result = this.UninstallPackage(package, options);
                    this.Write(StreamType.Object, new PSUninstallResult(result));
                });
        }

        private UninstallOptions GetUninstallOptions(
            PackageVersionId? version,
            PackageUninstallMode packageUninstallMode,
            bool force)
        {
            var options = ManagementDeploymentFactory.Instance.CreateUninstallOptions();
            options.Force = force;
            if (this.Log != null)
            {
                options.LogOutputPath = this.Log;
            }

            options.PackageUninstallMode = packageUninstallMode;

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

            var operation = PackageManagerWrapper.Instance.UninstallPackageAsync(package, options);

            var activityId = this.GetNewProgressActivityId();
            WriteProgressAdapter adapter = new (this);
            operation.Progress = (context, progress) =>
            {
                adapter.WriteProgress(new ProgressRecord(activityId, activity, progress.State.ToString())
                {
                    RecordType = ProgressRecordType.Processing,
                });
            };
            operation.Completed = (context, status) =>
            {
                adapter.WriteProgress(new ProgressRecord(activityId, activity, status.ToString())
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
