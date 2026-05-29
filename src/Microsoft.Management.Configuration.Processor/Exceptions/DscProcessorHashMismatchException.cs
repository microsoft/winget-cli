// -----------------------------------------------------------------------------
// <copyright file="DscProcessorHashMismatchException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// DSC processor does not match provided hash.
    /// </summary>
    internal class DscProcessorHashMismatchException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DscProcessorHashMismatchException"/> class.
        /// </summary>
        public DscProcessorHashMismatchException()
            : base("The DSC processor hash provided does not match hash of the target file.")
        {
            this.HResult = ErrorCodes.WinGetConfigProcessorHashMismatch;
        }
    }
}
