// -----------------------------------------------------------------------------
// <copyright file="TestResourceGetItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Implements IResourceGetItem for tests.
    /// </summary>
    internal class TestResourceGetItem : IResourceGetItem
    {
        /// <summary>
        /// Gets or sets the settings.
        /// </summary>
        public ValueSet Settings { get; set; } = new ValueSet();
    }
}
