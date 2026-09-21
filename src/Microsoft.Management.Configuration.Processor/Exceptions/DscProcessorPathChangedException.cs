// -----------------------------------------------------------------------------
// <copyright file="DscProcessorPathChangedException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;

    /// <summary>
    /// The DSC processor path no longer refers to the file that was verified.
    /// </summary>
    internal class DscProcessorPathChangedException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DscProcessorPathChangedException"/> class.
        /// </summary>
        public DscProcessorPathChangedException()
            : this("The DSC processor path no longer refers to the file that was verified.")
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DscProcessorPathChangedException"/> class.
        /// </summary>
        /// <param name="message">The message describing how the path differs from what was verified.</param>
        public DscProcessorPathChangedException(string message)
            : base(message)
        {
            this.HResult = ErrorCodes.WinGetConfigProcessorPathChanged;
        }
    }
}
