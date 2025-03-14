// -----------------------------------------------------------------------------
// <copyright file="TestResourceTestItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;

    /// <summary>
    /// Implements IResourceTestItem for tests.
    /// </summary>
    internal class TestResourceTestItem : IResourceTestItem
    {
        /// <summary>
        /// Gets or sets a value indicating whether the system is in the desired state.
        /// </summary>
        public bool InDesiredState { get; set; }
    }
}
