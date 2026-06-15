// -----------------------------------------------------------------------------
// <copyright file="UnitPropertyUnsupportedException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;

    /// <summary>
    /// The property type of a unit is not supported.
    /// </summary>
    internal class UnitPropertyUnsupportedException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="UnitPropertyUnsupportedException"/> class.
        /// </summary>
        /// <param name="name">Name.</param>
        /// <param name="type">Type.</param>
        /// <param name="inner">Inner exception.</param>
        public UnitPropertyUnsupportedException(string name, Type type, Exception inner)
            : base($"Property {name} of type {type.FullName} is not supported.", inner)
        {
            this.HResult = ErrorCodes.WinGetConfigUnitUnsupportedType;
            this.Name = name;
            this.Type = type;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="UnitPropertyUnsupportedException"/> class.
        /// </summary>
        /// <param name="type">Type.</param>
        public UnitPropertyUnsupportedException(Type type)
            : base($"Type {type.FullName} is not supported.")
        {
            this.HResult = ErrorCodes.WinGetConfigUnitUnsupportedType;
            this.Type = type;
        }

        /// <summary>
        /// Gets the name.
        /// </summary>
        public string? Name { get; }

        /// <summary>
        /// Gets the type.
        /// </summary>
        public Type Type { get; }
    }
}
