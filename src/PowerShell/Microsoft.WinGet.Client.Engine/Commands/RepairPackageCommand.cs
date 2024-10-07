// -----------------------------------------------------------------------------
// <copyright file="RepairPackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System;
    using System.Management.Automation;
    using System.Threading.Tasks;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// This class defines the repair package command.
    /// </summary>
    public sealed class RepairPackageCommand : PackageCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="RepairPackageCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">Caller cmdlet.</param>
        /// <param name="psCatalogPackage">PSCatalogPackage.</param>
        /// <param name="allowHashMismatch">To skip the installer hash validation check.</param>
        /// <param name="force">To continue upon non security related failures.</param>
        /// <param name="version">Version to repair.</param>
        /// <param name="log">Logging file location.</param>
        /// <param name="id">Package identifier.</param>
        /// <param name="name">Name of package.</param>
        /// <param name="moniker">Moniker of package.</param>
        /// <param name="source">Source to search. if null, all are searched.</param>
        /// <param name="query">Match against any field of a package.</param>
        public RepairPackageCommand(
            PSCmdlet psCmdlet,
            bool allowHashMismatch,
            bool force,
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
            this.Force = force;
            this.AllowHashMismatch = allowHashMismatch;

            if (psCatalogPackage != null)
            {
                this.CatalogPackage = psCatalogPackage;
            }

            this.Version = version;

            this.Id = id;
            this.Name = name;
            this.Moniker = moniker;
            this.Source = source;
            this.Query = query;

            this.Log = log;
        }

        /// <summary>
        /// Gets or sets the path to the logging file.
        /// </summary>
        private string? Log { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to continue upon non security related failures.
        /// </summary>
        private bool Force { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to skip the installer hash validation check.
        /// </summary>
        private bool AllowHashMismatch { get; set; }

        /// <summary>
        /// Process repair package.
        /// </summary>
        /// <param name="psPackageFieldMatchOption">PSPackageFieldMatchOption.</param>
        /// <param name="psPackageRepairMode">PSPackageRepairMode.</param>
        public void Repair(
            string psPackageFieldMatchOption,
            string psPackageRepairMode)
        {
            var result = this.Execute(
                async () => await this.GetPackageAndExecuteAsync(
                    CompositeSearchBehavior.AllCatalogs,
                    PSEnumHelpers.ToPackageFieldMatchOption(psPackageFieldMatchOption),
                    async (package, version) =>
                    {
                        var repairOptions = this.GetRepairOptions(version, PSEnumHelpers.ToPackageRepairMode(psPackageRepairMode));
                        return await this.RepairPackageAsync(package, repairOptions);
                    }));

            if (result != null)
            {
                if (result.Item1.Status == RepairResultStatus.RepairError
                    && result.Item1.ExtendedErrorCode != null)
                {
                    if (result.Item1.ExtendedErrorCode.InnerException != null)
                    {
                        throw new WinGetRepairPackageException(result.Item1.ExtendedErrorCode.HResult, result.Item1.RepairerErrorCode, result.Item1.ExtendedErrorCode.InnerException);
                    }

                    throw new WinGetRepairPackageException(result.Item1.ExtendedErrorCode.HResult, result.Item1.RepairerErrorCode);
                }

                this.Write(WinGet.Common.Command.StreamType.Object, new PSRepairResult(result.Item1, result.Item2));
            }
        }

        private RepairOptions GetRepairOptions(
            PackageVersionId? version,
            PackageRepairMode repairMode)
        {
            var options = ManagementDeploymentFactory.Instance.CreateRepairOptions();
            options.AllowHashMismatch = this.AllowHashMismatch;
            options.Force = this.Force;

            if (this.Log != null)
            {
                options.LogOutputPath = this.Log;
            }

            options.PackageRepairMode = repairMode;

            if (version != null)
            {
                options.PackageVersionId = version;
            }

            return options;
        }

        private async Task<RepairResult> RepairPackageAsync(
            CatalogPackage catalogPackage,
            RepairOptions repairOptions)
        {
            var progressOperation = new RepairOperationWithProgress(
                this,
                string.Format(Resources.ProgressRecordActivityRepairing, catalogPackage.Name));

            return await progressOperation.ExecuteAsync(
                () => PackageManagerWrapper.Instance.RepairPackageAsync(catalogPackage, repairOptions));
        }
    }
}
