// -----------------------------------------------------------------------------
// <copyright file="StringHelpers.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Helpers
{
    /// <summary>
    /// String helpers.
    /// </summary>
    internal static class StringHelpers
    {
        /// <summary>
        /// Normalize string.
        /// </summary>
        /// <param name="value">Value.</param>
        /// <returns>Normalized string.</returns>
        public static string Normalize(string value)
        {
            return value.ToLowerInvariant();
        }
    }
}
