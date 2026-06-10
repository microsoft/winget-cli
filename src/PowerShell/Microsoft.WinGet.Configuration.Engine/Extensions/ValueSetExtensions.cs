// -----------------------------------------------------------------------------
// <copyright file="ValueSetExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Extensions
{
    using Windows.Foundation.Collections;

    /// <summary>
    /// Extension methods for Value set.
    /// </summary>
    internal static class ValueSetExtensions
    {
        /// <summary>
        /// Gets the string value of a given key.
        /// Null is doesn't exist or cast can't be done.
        /// </summary>
        /// <param name="valueSet">Value set.</param>
        /// <param name="key">Key.</param>
        /// <returns>String value.</returns>
        public static string? TryGetStringValue(this ValueSet valueSet, string key)
        {
            if (valueSet.TryGetValue(key, out object value))
            {
                return value as string;
            }

            return null;
        }
    }
}
