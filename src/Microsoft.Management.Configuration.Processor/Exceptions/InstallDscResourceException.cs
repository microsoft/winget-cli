// -----------------------------------------------------------------------------
// <copyright file="InstallDscResourceException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;

    /// <summary>
    /// Installing a DSC resource failed unexpectedly.
    /// </summary>
    internal class InstallDscResourceException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InstallDscResourceException"/> class.
        /// </summary>
        /// <param name="resourceName">Resource name.</param>
        public InstallDscResourceException(string resourceName)
        {
            this.ResourceName = resourceName;
        }

        /// <summary>
        /// Gets the resource name.
        /// </summary>
        public string ResourceName { get; }
    }
}
