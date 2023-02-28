// -----------------------------------------------------------------------------
// <copyright file="PowerShellGetException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;

    /// <summary>
    /// PowerShellGet exception. Min version 2.2.5.
    /// </summary>
    internal class PowerShellGetException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PowerShellGetException"/> class.
        /// </summary>
        public PowerShellGetException()
        {
        }
    }
}
