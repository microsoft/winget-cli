// -----------------------------------------------------------------------------
// <copyright file="ErrorCodes.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.SharedLib.Exceptions
{
    /// <summary>
    /// This should match the ones in AppInstallerErrors.h.
    /// </summary>
    internal static class ErrorCodes
    {
#pragma warning disable SA1600 // ElementsMustBeDocumented
        internal const int AppInstallerCLIErrorInternalError = unchecked((int)0x8A150001);
        internal const int AppInstallerCLIErrorBlockedByPolicy = unchecked((int)0x8A15003A);
#pragma warning restore SA1600 // ElementsMustBeDocumented
    }
}
