// -----------------------------------------------------------------------------
// <copyright file="ConfigurationEnvironmentData.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Contains the data defining a configuration environment.
    /// </summary>
    internal class ConfigurationEnvironmentData
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationEnvironmentData"/> class.
        /// </summary>
        internal ConfigurationEnvironmentData()
        {
        }

        /// <summary>
        /// Gets or sets the security context.
        /// </summary>
        internal SecurityContext Context { get; set; } = SecurityContext.Current;

        /// <summary>
        /// Gets or sets the processor identifier.
        /// </summary>
        internal string ProcessorIdentifier { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets the processor properties.
        /// </summary>
        internal Dictionary<string, string> ProcessorProperties { get; set; } = new ();

        /// <summary>
        /// Applies this environment to the given unit.
        /// </summary>
        /// <param name="unit">The unit to apply to.</param>
        /// <returns>The given unit.</returns>
        internal ConfigurationUnit ApplyToUnit(ConfigurationUnit unit)
        {
            var environment = unit.Environment;

            environment.Context = this.Context;
            environment.ProcessorIdentifier = this.ProcessorIdentifier;
            environment.ProcessorProperties.Clear();
            foreach (var property in this.ProcessorProperties)
            {
                environment.ProcessorProperties.Add(property.Key, property.Value);
            }

            return unit;
        }

        /// <summary>
        /// Tests whether the given properties match this object's properties.
        /// </summary>
        /// <param name="properties">The properties to test.</param>
        /// <returns>True if the properties match; false if not.</returns>
        internal bool PropertiesEqual(IDictionary<string, string> properties)
        {
            if (properties.Count != this.ProcessorProperties.Count)
            {
                return false;
            }

            foreach (var property in properties)
            {
                string? value = null;
                if (!this.ProcessorProperties.TryGetValue(property.Key, out value))
                {
                    return false;
                }

                if (property.Value != value)
                {
                    return false;
                }
            }

            return true;
        }
    }
}
