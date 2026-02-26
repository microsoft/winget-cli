// -----------------------------------------------------------------------------
// <copyright file="IResourceTestItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Model
{
    /// <summary>
    /// The interface to a `resource test` command result.
    /// </summary>
    internal interface IResourceTestItem
    {
        /// <summary>
        /// Gets a value indicating whether the resource is in the desired state.
        /// </summary>
        public bool InDesiredState { get; }
    }
}
