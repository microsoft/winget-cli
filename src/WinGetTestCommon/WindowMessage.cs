// -----------------------------------------------------------------------------
// <copyright file="WindowMessage.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetTestCommon
{
    /// <summary>
    /// Represents the Windows messages that can be sent or received by a window.
    /// </summary>
    public enum WindowMessage
    {
        /// <summary>
        /// WM_CLOSE
        /// </summary>
        Close = 0x0010,

        /// <summary>
        /// WM_QUERYENDSESSION
        /// </summary>
        QueryEndSession = 0x0011,

        /// <summary>
        /// WM_ENDSESSION
        /// </summary>
        EndSession = 0x0016,
    }
}
