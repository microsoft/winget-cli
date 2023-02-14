// -----------------------------------------------------------------------------
// <copyright file="FindDscResourceNotFoundException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;

    /// <summary>
    /// Resource not found by Find-DscResource.
    /// </summary>
    internal class FindDscResourceNotFoundException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FindDscResourceNotFoundException"/> class.
        /// </summary>
        public FindDscResourceNotFoundException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FindDscResourceNotFoundException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        public FindDscResourceNotFoundException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FindDscResourceNotFoundException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>
        /// <param name="inner">Inner exception.</param>
        public FindDscResourceNotFoundException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}
