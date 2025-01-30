// -----------------------------------------------------------------------------
// <copyright file="DictionaryExtensions.cs" company="Microsoft Corporation">
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
    /// Extensions for dictionaries.
    /// </summary>
    internal static class DictionaryExtensions
    {
        /// <summary>
        /// Performs a deep compare of the dictionaries.
        /// </summary>
        /// <param name="first">First dictionary.</param>
        /// <param name="second">Second dictionary.</param>
        /// <returns>Whether the two dictionaries equal.</returns>
        internal static bool ContentEquals(this IDictionary<string, string> first, IDictionary<string, string> second)
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

                if (firstValue != secondValue)
                {
                    return false;
                }
            }

            return true;
        }
    }
}
