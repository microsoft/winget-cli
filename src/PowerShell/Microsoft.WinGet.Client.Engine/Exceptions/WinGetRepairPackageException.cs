// -----------------------------------------------------------------------------
// <copyright file="WinGetRepairPackageException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// WinGetRepairPackageException.
    /// </summary>
    [Serializable]
    public class WinGetRepairPackageException : RuntimeException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetRepairPackageException"/> class.
        /// </summary>
        /// <param name="hresult">Repair operation ExtendedErrorCode Hresult.</param>
        public WinGetRepairPackageException(int hresult)
            : base(GetMessage(hresult))
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetRepairPackageException"/> class.
        /// </summary>
        /// <param name="hresult">Repair operation ExtendedErrorCode Hresult.</param>
        /// <param name="repairerExitCode">Repairer exit code.</param>
        public WinGetRepairPackageException(int hresult, uint repairerExitCode)
            : base(GetMessage(hresult, repairerExitCode))
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetRepairPackageException"/> class.
        /// </summary>
        /// <param name="hresult">Repair operation ExtendedErrorCode Hresult.</param>
        /// <param name="innerException">InnerException.</param>
        public WinGetRepairPackageException(int hresult, Exception innerException)
            : base(GetMessage(hresult), innerException)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetRepairPackageException"/> class.
        /// </summary>
        /// <param name="hresult">Repair operation ExtendedErrorCode Hresult.</param>
        /// <param name="repairerExitCode">Repairer exit code.</param>
        /// <param name="innerException">InnerException.</param>
        public WinGetRepairPackageException(int hresult, uint repairerExitCode, Exception innerException)
            : base(GetMessage(hresult, repairerExitCode), innerException)
        {
        }

        private static string GetMessage(int hresult, uint repairerExitCode = 0)
        {
            switch (hresult)
            {
                case ErrorCode.NoRepairInfoFound:
                    return Resources.NoRepairInfoFound;
                case ErrorCode.RepairerFailure:
                    return string.Format(Resources.RepairerFailure, repairerExitCode);
                case ErrorCode.RepairNotSupported:
                    return Resources.RepairOperationNotSupported;
                case ErrorCode.RepairNotApplicable:
                    return Resources.RepairDifferentInstallTechnology;
                case ErrorCode.AdminContextRepairProhibited:
                    return Resources.NoAdminRepairForUserScopePackage;
                default:
                    return string.Format(Resources.UnknownRepairFailure, hresult);
            }
        }
    }
}
