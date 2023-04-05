// -----------------------------------------------------------------------------
// <copyright file="ValueSetExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Extensions
{
    using System.Collections;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Extensions for ValueSet.
    /// </summary>
    internal static class ValueSetExtensions
    {
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
                    hashtable.Add(keyValuePair.Key, innerValueSet.ToHashtable());
                }
                else
                {
                    hashtable.Add(keyValuePair.Key, keyValuePair.Value);
                }
            }

            return hashtable;
        }
    }
}
