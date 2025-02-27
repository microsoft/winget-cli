// -----------------------------------------------------------------------------
// <copyright file="IResourceSetItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Model
{
    using System.Collections.Generic;

    /// <summary>
    /// The interface to a `resource set` command result.
    /// </summary>
    internal interface IResourceSetItem
    {
        /// <summary>
        /// Gets a value indicating whether a reboot is required.
        /// </summary>
        public bool RebootRequired { get; }
    }
}
