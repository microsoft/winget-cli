// -----------------------------------------------------------------------------
// <copyright file="HashtableExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Extensions
{
    using System.Collections;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Extensions for Hashtable.
    /// </summary>
    internal static class HashtableExtensions
    {
        /// <summary>
        /// Convert a hashtable to a value set.
        /// </summary>
        /// <param name="hashtable">hashtable.</param>
        /// <returns>Value set.</returns>
        public static ValueSet ToValueSet(this Hashtable hashtable)
        {
            var valueSet = new ValueSet();

            foreach (DictionaryEntry entry in hashtable)
            {
                if (entry.Key is string key)
                {
                    if (entry.Value is null)
                    {
                        valueSet.Add(key, null);
                    }
                    else
                    {
                        var value = TypeHelpers.GetCompatibleValueSetValueOfProperty(entry.Value.GetType(), entry.Value);
                        if (value != null)
                        {
                            valueSet.Add(key, value);
                        }
                    }
                }
                else
                {
                    throw new UnitPropertyUnsupportedException(entry.Key.GetType());
                }
            }

            return valueSet;
        }
    }
}
