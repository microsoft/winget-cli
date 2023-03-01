// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorFactoryProperties.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    using System.Collections.Generic;

    /// <summary>
    /// Implementation of <see cref="IConfigurationProcessorFactoryProperties"/>.
    /// </summary>
    public sealed class ConfigurationProcessorFactoryProperties : IConfigurationProcessorFactoryProperties
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationProcessorFactoryProperties"/> class.
        /// </summary>
        public ConfigurationProcessorFactoryProperties()
        {
        }

        /// <inheritdoc/>
        public IReadOnlyList<string>? AdditionalModulePaths { get; set; }
    }
}
