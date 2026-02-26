// -----------------------------------------------------------------------------
// <copyright file="ConfigurationExtensions.cs" company="Microsoft Corporation">
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
        internal static ConfigurationUnit Assign(this ConfigurationUnit unit, object properties)
        {
            PropertyInfo[] unitProperties = typeof(ConfigurationUnit).GetProperties();

            foreach (PropertyInfo property in properties.GetType().GetProperties())
            {
                PropertyInfo matchingProperty = unitProperties.First(pi => pi.Name == property.Name);
                matchingProperty.SetValue(unit, property.GetValue(properties));
            }

            return unit;
        }
    }
}
