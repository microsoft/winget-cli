// -----------------------------------------------------------------------------
// <copyright file="UnitPropertyUnsupportedException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;
    using System.Reflection;

    /// <summary>
    /// The property type of a unit is not supported.
    /// </summary>
    internal class UnitPropertyUnsupportedException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="UnitPropertyUnsupportedException"/> class.
        /// </summary>
        /// <param name="property">PropertyInfo.</param>
        /// <param name="inner">Inner exception.</param>
        public UnitPropertyUnsupportedException(PropertyInfo property, Exception inner)
            : base($"Property {property.Name} of type {property.PropertyType.FullName} is not supported.", inner)
        {
            this.HResult = ErrorCodes.WinGetConfigUnitUnsupportedType;
            this.Property = property;
        }

        /// <summary>
        /// Gets the property.
        /// </summary>
        public PropertyInfo Property { get; }
    }
}
