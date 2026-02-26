// -----------------------------------------------------------------------------
// <copyright file="ErrorCode.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Common
{
    /// <summary>
    /// Error code constants.
    /// </summary>
    internal static class ErrorCode
    {
        /// <summary>
        /// Error code for ERROR_FILE_NOT_FOUND.
        /// </summary>
        public const int FileNotFound = unchecked((int)0x80070002);

        /// <summary>
        /// Error code for RPC_S_SERVER_UNAVAILABLE.
        /// </summary>
        public const int RpcServerUnavailable = unchecked((int)0x800706BA);

        /// <summary>
        /// Error code for RPC_S_CALL_FAILED.
        /// </summary>
        public const int RpcCallFailed = unchecked((int)0x800706BE);

        /// <summary>
        /// Error code for ERROR_PACKAGE_NOT_REGISTERED_FOR_USER.
        /// </summary>
        public const int PackageNotRegisteredForUser = unchecked((int)0x80073D35);

        /// <summary>
        /// Error code for APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND.
        /// </summary>
        public const int NoRepairInfoFound = unchecked((int)0x8A150079);

        /// <summary>
        /// Error code for APPINSTALLER_CLI_ERROR_REPAIR_NOT_APPLICABLE.
        /// </summary>
        public const int RepairNotApplicable = unchecked((int)0x8A15007A);

        /// <summary>
        /// Error code for APPINSTALLER_CLI_ERROR_EXEC_REPAIR_FAILED .
        /// </summary>
        public const int RepairerFailure = unchecked((int)0x8A15007B);

        /// <summary>
        /// Error code for APPINSTALLER_CLI_ERROR_REPAIR_NOT_SUPPORTED.
        /// </summary>
        public const int RepairNotSupported = unchecked((int)0x8A15007C);

        /// <summary>
        /// Error code for APPINSTALLER_CLI_ERROR_ADMIN_CONTEXT_REPAIR_PROHIBITED.
        /// </summary>
        public const int AdminContextRepairProhibited = unchecked((int)0x8A15007D);
    }
}
