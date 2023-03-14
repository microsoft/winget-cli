// -----------------------------------------------------------------------------
// <copyright file="IConfigurationProcessorFactoryProperties.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    using System.Collections.Generic;

    /// <summary>
    /// Properties for the configuration processor factory.
    /// </summary>
    public interface IConfigurationProcessorFactoryProperties
    {
        /// <summary>
        /// Gets or sets the additional module paths.
        /// </summary>
        IReadOnlyList<string>? AdditionalModulePaths { get; set; }
    }
}
