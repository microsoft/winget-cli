// -----------------------------------------------------------------------------
// <copyright file="HashtableExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Extensions
{
    using System.Collections;
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
                if (entry.Value == null)
                {
                    continue;
                }

                // Ignore keys that are not strings.
                string? key = entry.Key as string;
                if (key != null)
                {
                    if (entry.Value.GetType() == typeof(Hashtable))
                    {
                        var innerHashtable = (Hashtable)entry.Value;
                        valueSet.Add(key, innerHashtable.ToValueSet());
                    }
                    else if (entry.Value.GetType().IsEnum == true)
                    {
                        valueSet.Add(key, entry.Value.ToString());
                    }
                    else
                    {
                        valueSet.Add(key, entry.Value);
                    }
                }
            }

            return valueSet;
        }
    }
}
