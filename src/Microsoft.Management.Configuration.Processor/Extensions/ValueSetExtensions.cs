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
    using WinRT;

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
    }
}
