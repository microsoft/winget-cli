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
                var key = property.Name;
                var value = GetCompatibleValueSetValueOfProperty(property.PropertyType, property.GetValue(obj));
                result.Add(key, value);
            }

            return result;
        }

        /// <summary>
        /// Gets a compatible type for a ValueSet value.
        /// </summary>
        /// <param name="type">Type.</param>
        /// <param name="value">Value.</param>
        /// <returns>Value converted to a compatible type.</returns>
        public static object? GetCompatibleValueSetValueOfProperty(Type type, object? value)
        {
            if (value == null)
            {
                return null;
            }

            // Specialize here.
            if (type.IsEnum)
            {
                return value.ToString();
            }
            else if (type == typeof(Hashtable))
            {
                Hashtable hashtable = (Hashtable)value;
                return hashtable.ToValueSet();
            }
            else if (type.IsArray)
            {
                var valueSetArray = new ValueSet();
                int index = 0;
                foreach (object arrayObj in (Array)value)
                {
                    var arrayValue = GetCompatibleValueSetValueOfProperty(arrayObj.GetType(), arrayObj);
                    if (arrayValue != null)
                    {
                        valueSetArray.Add(index.ToString(), arrayValue);
                        index++;
                    }
                }

                if (valueSetArray.Count > 0)
                {
                    valueSetArray.Add("treatAsArray", true);
                }

                return valueSetArray;
            }
            else if (type == typeof(string))
            {
                // Ignore empty strings.
                string propertyString = (string)value;
                if (!string.IsNullOrEmpty(propertyString))
                {
                    return propertyString;
                }
                else
                {
                    return null;
                }
            }
            else if (type.IsValueType)
            {
                return value;
            }

            // This might be too restrictive but anything else is going to be some object that we don't support anyway.
            throw new UnitPropertyUnsupportedException(value.GetType());
        }
    }
}
