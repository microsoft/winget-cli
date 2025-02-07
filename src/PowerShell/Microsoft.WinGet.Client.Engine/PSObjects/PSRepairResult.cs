// -----------------------------------------------------------------------------
// <copyright file="PSRepairResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    using System;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// PSRepairResult.
    /// </summary>
    public sealed class PSRepairResult
    {
        private readonly RepairResult repairResult;
        private readonly CatalogPackage catalogPackage;

        /// <summary>
        /// Initializes a new instance of the <see cref="PSRepairResult"/> class.
        /// </summary>
        /// <param name="repairResult">The Repair result COM Object.</param>
        /// <param name="catalogPackage">The catalog package COM Object.</param>
        internal PSRepairResult(RepairResult repairResult, CatalogPackage catalogPackage)
        {
            this.repairResult = repairResult;
            this.catalogPackage = catalogPackage;
        }

        /// <summary>
        /// Gets the id of the repaired package.
        /// </summary>
        public string Id
        {
            get
            {
                return this.catalogPackage.Id;
            }
        }

        /// <summary>
        /// Gets the name of the repaired package.
        /// </summary>
        public string Name
        {
            get
            {
                return this.catalogPackage.Name;
            }
        }

        /// <summary>
        /// Gets the source name of the repaired package.
        /// </summary>
        public string Source
        {
            get
            {
                return this.catalogPackage.DefaultInstallVersion.PackageCatalog.Info.Name;
            }
        }

        /// <summary>
        /// Gets the correlation data of the repair result.
        /// </summary>
        public string CorrelationData
        {
            get
            {
                return this.repairResult.CorrelationData;
            }
        }

        /// <summary>
        /// Gets the extended error code exception of the failed repair result.
        /// </summary>
        public Exception ExtendedErrorCode
        {
            get
            {
                return this.repairResult.ExtendedErrorCode;
            }
        }

        /// <summary>
        /// Gets a value indicating whether a reboot is required.
        /// </summary>
        public bool RebootRequired
        {
            get
            {
                return this.repairResult.RebootRequired;
            }
        }

        /// <summary>
        /// Gets the error code of a repair.
        /// </summary>
        public uint RepairErrorCode
        {
            get
            {
                return this.repairResult.RepairerErrorCode;
            }
        }

        /// <summary>
        /// Gets the status of the repair result.
        /// </summary>
        public string Status
        {
            get
            {
                return this.repairResult.Status.ToString();
            }
        }

        /// <summary>
        /// If the repair succeeded.
        /// </summary>
        /// <returns>True if repair succeeded.</returns>
        public bool Succeeded()
        {
            return this.repairResult.Status == RepairResultStatus.Ok;
        }

        /// <summary>
        /// Message with error information.
        /// </summary>
        /// <returns>Error message.</returns>
        public string ErrorMessage()
        {
            return $"RepairStatus : '{this.Status}' RepairErrorCode: '{this.RepairErrorCode}' ExtendedError: '{this.ExtendedErrorCode.HResult}'";
        }
    }
}
