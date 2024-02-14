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
    }
}
