// -----------------------------------------------------------------------------
// <copyright file="ValueSetExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Extensions
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Extensions for ValueSet.
    /// </summary>
    internal static class ValueSetExtensions
    {
        private const string TreatAsArray = "treatAsArray";

        /// <summary>
        /// Extension method to transform a ValueSet to a Hashtable.
        /// </summary>
        /// <param name="valueSet">Value set.</param>
        /// <returns>A hashtable.</returns>
        public static Hashtable ToHashtable(this ValueSet valueSet)
        {
            var hashtable = new Hashtable();

            foreach (var keyValuePair in valueSet)
            {
                if (keyValuePair.Value is ValueSet innerValueSet)
                {
                    if (innerValueSet.ContainsKey(TreatAsArray))
                    {
                        hashtable.Add(keyValuePair.Key, innerValueSet.ToArray());
                    }
                    else
                    {
                        hashtable.Add(keyValuePair.Key, innerValueSet.ToHashtable());
                    }
                }
                else
                {
                    hashtable.Add(keyValuePair.Key, keyValuePair.Value);
                }
            }

            return hashtable;
        }

        /// <summary>
        /// Gets ordered list from a ValueSet that is threated as an array.
        /// </summary>
        /// <param name="valueSet">ValueSet.</param>
        /// <returns>Ordered list.</returns>
        public static IList<object> ToArray(this ValueSet valueSet)
        {
            if (!valueSet.ContainsKey(TreatAsArray))
            {
                throw new InvalidOperationException();
            }

            var sortedList = new SortedList<int, object>();

            foreach (var keyValuePair in valueSet)
            {
                if (keyValuePair.Key == TreatAsArray)
                {
                    continue;
                }

                if (int.TryParse(keyValuePair.Key, out int key))
                {
                    if (keyValuePair.Value is ValueSet innerValueSet)
                    {
                        sortedList.Add(key, innerValueSet.ToHashtable());
                    }
                    else
                    {
                        sortedList.Add(key, keyValuePair.Value);
                    }
                }
                else
                {
                    throw new InvalidOperationException($"Invalid key for ValueSet to array {keyValuePair.Key}");
                }
            }

            return sortedList.Values;
        }

        /// <summary>
        /// Performs a deep compare of the ValueSets.
        /// </summary>
        /// <param name="first">First ValueSet.</param>
        /// <param name="second">Second ValueSet.</param>
        /// <returns>Whether the two ValueSets equal.</returns>
        public static bool ContentEquals(this ValueSet first, ValueSet second)
        {
            if (first.Count != second.Count)
            {
                return false;
            }

            foreach (var keyValuePair in first)
            {
                string key = keyValuePair.Key;
                if (!second.ContainsKey(key))
                {
                    return false;
                }

                var firstValue = keyValuePair.Value;
                var secondValue = second[key];

                // Empty value check.
                if (firstValue == null && secondValue == null)
                {
                    continue;
                }
                else if (firstValue == null || secondValue == null)
                {
                    return false;
                }

                // Try as ValueSet.
                var firstValueSet = firstValue as ValueSet;
                var secondValueSet = secondValue as ValueSet;

                if (firstValueSet != null && secondValueSet != null)
                {
                    if (!firstValueSet.ContentEquals(secondValueSet))
                    {
                        return false;
                    }
                    else
                    {
                        continue;
                    }
                }
                else if (firstValueSet != null || secondValueSet != null)
                {
                    return false;
                }

                // Try as scalar.
                if (firstValue is string firstString && secondValue is string secondString)
                {
                    if (firstString != secondString)
                    {
                        return false;
                    }
                }
                else if (firstValue is long firstLong && secondValue is long secondLong)
                {
                    if (firstLong != secondLong)
                    {
                        return false;
                    }
                }
                else if (firstValue is bool firstBool && secondValue is bool secondBool)
                {
                    if (firstBool != secondBool)
                    {
                        return false;
                    }
                }
                else
                {
                    // Note: DateTime and float are not supported in parser yet.
                    return false;
                }
            }

            return true;
        }
    }
}
