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
                if (keyValuePair.Value is ValueSet)
                {
                    ValueSet innerValueSet = (ValueSet)keyValuePair.Value;
                    if (innerValueSet.ContainsKey(TreatAsArray))
                    {
                        hashtable.Add(keyValuePair.Key, ConvertValueSetToArray(innerValueSet));
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

        private static List<object> ConvertValueSetToArray(ValueSet valueSet)
        {
            if (!valueSet.ContainsKey(TreatAsArray))
            {
                throw new InvalidOperationException();
            }

            var result = new List<object>();

            foreach (var keyValuePair in valueSet)
            {
                if (keyValuePair.Key == TreatAsArray)
                {
                    continue;
                }

                if (keyValuePair.Value is ValueSet)
                {
                    ValueSet innerValueSet = (ValueSet)keyValuePair.Value;
                    result.Add(innerValueSet.ToHashtable());
                }
                else
                {
                    result.Add(keyValuePair.Value);
                }
            }

            return result;
        }
    }
}
