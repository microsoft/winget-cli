// -----------------------------------------------------------------------------
// <copyright file="TestResourceSetItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;

    /// <summary>
    /// Implements IResourceSetItem for tests.
    /// </summary>
    internal class TestResourceSetItem : IResourceSetItem
    {
        /// <summary>
        /// Gets or sets a value indicating whether a reboot is required.
        /// </summary>
        public bool RebootRequired { get; set; }
    }
}
