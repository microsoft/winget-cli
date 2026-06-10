// -----------------------------------------------------------------------------
// <copyright file="WindowsPowerShellNotSupported.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Windows PowerShell is not supported.
    /// </summary>
    [Serializable]
    public class WindowsPowerShellNotSupported : RuntimeException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WindowsPowerShellNotSupported"/> class.
        /// </summary>
        public WindowsPowerShellNotSupported()
            : base(Resources.WindowsPowerShellNotSupported)
        {
        }
    }
}
