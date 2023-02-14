// -----------------------------------------------------------------------------
// <copyright file="GetDscResourceNotFoundException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;

    /// <summary>
    /// Resource not found by Get-DscResource.
    /// </summary>
    internal class GetDscResourceNotFoundException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GetDscResourceNotFoundException"/> class.
        /// </summary>
        public GetDscResourceNotFoundException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="GetDscResourceNotFoundException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        public GetDscResourceNotFoundException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="GetDscResourceNotFoundException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        /// <param name="inner">Inner exception.</param>
        public GetDscResourceNotFoundException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}
