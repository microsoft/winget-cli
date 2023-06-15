// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorTestBase.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System.Linq;
    using System.Reflection;

    /// <summary>
    /// Contains extension methods for configuration objects.
    /// </summary>
    internal static class ConfigurationExtensions
    {
        /// <summary>
        /// Assigns the given properties to the configuration unit.
        /// </summary>
        /// <param name="unit">The unit to assign the properties of.</param>
        /// <param name="properties">The properties to assign.</param>
        /// <returns>The given ConfigurationUnit.</returns>
        internal static Configuration.ConfigurationUnit Assign(this Configuration.ConfigurationUnit unit, object properties)
        {
            PropertyInfo[] unitProperties = typeof(Configuration.ConfigurationUnit).GetProperties();

            foreach (PropertyInfo property in properties.GetType().GetProperties())
            {
                PropertyInfo matchingProperty = unitProperties.First(pi => pi.Name == property.Name);
                matchingProperty.SetValue(unit, property.GetValue(properties));
            }

            return unit;
        }

        /// <summary>
        /// Gets the unit result from the apply set result matching the given unit.
        /// </summary>
        /// <param name="setResult">The set result to inspect.</param>
        /// <param name="unit">The unit to match.</param>
        /// <returns>The unit result matching the unit, or null if not found.</returns>
        internal static Configuration.ApplyConfigurationUnitResult? GetUnitResultFor(this Configuration.ApplyConfigurationSetResult setResult, Configuration.ConfigurationUnit unit)
        {
            foreach (Configuration.ApplyConfigurationUnitResult unitResult in setResult.UnitResults)
            {
                if (unitResult.Unit.Identifier == unit.Identifier)
                {
                    return unitResult;
                }
            }

            return null;
        }
    }
}
