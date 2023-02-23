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
        /// <param name="additionalModulePaths">Additional module paths.</param>
        public ConfigurationProcessorFactoryProperties(IReadOnlyList<string> additionalModulePaths)
        {
            this.AdditionalModulePaths = additionalModulePaths;
        }

        /// <inheritdoc/>
        public IReadOnlyList<string> AdditionalModulePaths { get; }
    }
}
