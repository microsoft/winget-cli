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
        /// <param name="unitName">Unit name.</param>
        public FindDscResourceNotFoundException(string unitName)
        {
            this.HResult = ErrorCodes.WinGetConfigUnitNotFoundRepository;
            this.UnitName = unitName;
        }

        /// <summary>
        /// Gets the unit name.
        /// </summary>
        public string UnitName { get; }
    }
}
