// -----------------------------------------------------------------------------
// <copyright file="ExceptionExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Extensions
{
    using System;

    /// <summary>
    /// Extension method for Exception.
    /// </summary>
    internal static class ExceptionExtensions
    {
        /// <summary>
        /// Gets the most inner exception.
        /// </summary>
        /// <param name="e">Exception.</param>
        /// <returns>Most inner exception.</returns>
        public static Exception GetMostInnerException(this Exception e)
        {
            Exception ex = e;
            while (ex.InnerException != null)
            {
                ex = ex.InnerException;
            }

            return ex;
        }
    }
}
