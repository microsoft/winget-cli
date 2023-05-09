// -----------------------------------------------------------------------------
// <copyright file="IConfigurationUnitResultException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// An interface that enables an exception to expose information appropriate for a unit result.
    /// </summary>
    internal interface IConfigurationUnitResultException
    {
        /// <summary>
        /// Gets a value indicating the source of the result.
        /// </summary>
        public ConfigurationUnitResultSource ResultSource { get; }

        /// <summary>
        /// Gets the description of the result.
        /// </summary>
        public string Description { get; }

        /// <summary>
        /// Gets the details for the result.
        /// </summary>
        public string Details { get; }
    }
}
