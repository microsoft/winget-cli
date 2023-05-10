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
    using Microsoft.WinGet.Client.Engine.Properties;
    using Microsoft.WinGet.Client.Engine.PSObjects;

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
        /// <param name="matchOption">Match option.</param>
        public UninstallPackageCommand(
            PSCmdlet psCmdlet,
            PSCatalogPackage psCatalogPackage,
            string version,
            string log,
            string id,
            string name,
            string moniker,
            string source,
            string[] query,
            string matchOption)
            : base(psCmdlet)
        {
#if POWERSHELL_WINDOWS
            throw new NotSupportedException(Resources.WindowsPowerShellNotSupported);
#else
            // PackageCommand.
            if (psCatalogPackage != null)
            {
                this.CatalogPackage = psCatalogPackage.CatalogPackageCOM;
            }

            this.Version = version;
            this.Log = log;

            // FinderCommand
            this.Id = id;
            this.Name = name;
            this.Moniker = moniker;
            this.Source = source;
            this.Query = query;
            this.MatchOption = PSEnumHelpers.ToPackageFieldMatchOption(matchOption);
#endif
        }

        /// <summary>
        /// Process uninstall package.
        /// </summary>
        /// <param name="psPackageUninstallMode">PSPackageUninstallMode.</param>
        /// <param name="force">Force.</param>
        public void Uninstall(
            string psPackageUninstallMode,
            bool force)
        {
            this.GetPackageAndExecute(CompositeSearchBehavior.LocalCatalogs, (package, version) =>
            {
                UninstallOptions options = this.GetUninstallOptions(version, PSEnumHelpers.ToPackageUninstallMode(psPackageUninstallMode), force);
                UninstallResult result = this.UninstallPackage(package, options);
                this.PsCmdlet.WriteObject(new PSUninstallResult(result));
            });
        }

        private UninstallOptions GetUninstallOptions(
            PackageVersionId version,
            PackageUninstallMode packageUninstallMode,
            bool force)
        {
            var options = ComObjectFactory.Value.CreateUninstallOptions();
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

            var operation = PackageManager.Value.UninstallPackageAsync(package, options);
            WriteProgressAdapter adapter = new (this.PsCmdlet);
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
