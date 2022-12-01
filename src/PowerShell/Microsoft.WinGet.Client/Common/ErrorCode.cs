// -----------------------------------------------------------------------------
// <copyright file="ErrorCode.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Common
{
    /// <summary>
    /// Error code constants.
    /// </summary>
    public class ErrorCode
    {
        /// <summary>
        /// Error code for FILENOTFOUND.
        /// </summary>
        public const int FILENOTFOUND = unchecked((int)0x80070002);
    }
}
