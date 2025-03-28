// -----------------------------------------------------------------------------
// <copyright file="TestResourceExportItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Implements IResourceExportItem for tests.
    /// </summary>
    internal class TestResourceExportItem : IResourceExportItem
    {
        /// <summary>
        /// Gets or sets the type.
        /// </summary>
        required public string Type { get; set; }

        /// <summary>
        /// Gets or sets the name.
        /// </summary>
        required public string Name { get; set; }

        /// <summary>
        /// Gets or sets the settings.
        /// </summary>
        public ValueSet Settings { get; set; } = new ValueSet();

        /// <summary>
        /// Gets or sets the metadata.
        /// </summary>
        public ValueSet Metadata { get; set; } = new ValueSet();

        /// <summary>
        /// Gets or sets the dependencies.
        /// </summary>
        public IList<string> Dependencies { get; set; } = new List<string>();
    }
}
