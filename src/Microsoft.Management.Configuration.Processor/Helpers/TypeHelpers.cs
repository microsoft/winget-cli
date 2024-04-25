// -----------------------------------------------------------------------------
// <copyright file="TypeHelpers.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Helpers
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Reflection;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Type helpers.
    /// </summary>
    internal static class TypeHelpers
    {
        /// <summary>
        /// Verifies a property exists.
        /// </summary>
        /// <param name="obj">Dynamic object.</param>
        /// <param name="name">Name of property.</param>
        /// <returns>True if property exists.</returns>
        public static bool PropertyExists(dynamic obj, string name)
        {
            return obj.GetType().GetProperty(name) is not null;
        }

        /// <summary>
        /// Verifies a property exists with the specified type.
        /// </summary>
        /// <typeparam name="TType">Expected type.</typeparam>
        /// <param name="obj">Dynamic object.</param>
        /// <param name="name">Name of property.</param>
        /// <returns>True if property and is of the specified type.</returns>
        public static bool PropertyWithTypeExists<TType>(dynamic obj, string name)
        {
            return PropertyExists(obj, name) &&
                   obj.GetType().GetProperty(name).PropertyType == typeof(TType);
        }

        /// <summary>
        /// Verifies the property exist and is an enum.
        /// </summary>
        /// <param name="obj">Dynamic object.</param>
        /// <param name="name">Name of property.</param>
        /// <returns>True if property exists and is an enum.</returns>
        public static bool PropertyExistsAndIsEnum(dynamic obj, string name)
        {
            return PropertyExists(obj, name) &&
                   obj.GetType().GetProperty(name).PropertyType.IsEnum;
        }

        /// <summary>
        /// Verifies the property exists and is a list. Use this when you don't know the type of the List.
        /// </summary>
        /// <param name="obj">Dynamic object.</param>
        /// <param name="name">Name of property.</param>
        /// <returns>True if property exists and is a list.</returns>
        public static bool PropertyExistsAndIsList(dynamic obj, string name)
        {
            return PropertyExists(obj, name) &&
                   obj.GetType().GetProperty(name).PropertyType.IsGenericType &&
                   obj.GetType().GetProperty(name).PropertyType.GetGenericTypeDefinition() == typeof(List<>);
        }

        /// <summary>
        /// Gets all the properties and values from an object.
        /// </summary>
        /// <param name="obj">Object.</param>
        /// <returns>ValueSet with properties names and values.</returns>
        public static ValueSet GetAllPropertiesValues(object obj)
        {
            var result = new ValueSet();
            foreach (PropertyInfo property in obj.GetType().GetProperties())
            {
                try
                {
                    var propertyValue = property.GetValue(obj);
                    if (propertyValue == null)
                    {
                        // Ignore null values.
                        continue;
                    }

                    // Specialize here.
                    if (property.PropertyType.IsEnum)
                    {
                        result.Add(property.Name, propertyValue.ToString());
                    }
                    else if (property.PropertyType == typeof(Hashtable))
                    {
                        Hashtable hashtable = (Hashtable)propertyValue;
                        result.Add(property.Name, hashtable.ToValueSet());
                    }
                    else if (property.PropertyType == typeof(string))
                    {
                        string propertyString = (string)propertyValue;
                        if (!string.IsNullOrEmpty(propertyString))
                        {
                            result.Add(property.Name, propertyString);
                        }
                    }
                    else
                    {
                        result.Add(property.Name, property.GetValue(obj));
                    }
                }
                catch (Exception e)
                {
                    throw new UnitPropertyUnsupportedException(property, e);
                }
            }

            return result;
        }
    }
}
