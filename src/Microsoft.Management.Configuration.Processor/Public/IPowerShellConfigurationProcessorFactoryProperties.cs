// -----------------------------------------------------------------------------
// <copyright file="IPowerShellConfigurationProcessorFactoryProperties.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    using System.Collections.Generic;

    /// <summary>
    /// Properties for the configuration processor factory.
    /// </summary>
    public interface IPowerShellConfigurationProcessorFactoryProperties
    {
        /// <summary>
        /// Gets or sets the processor type.
        /// </summary>
        PowerShellConfigurationProcessorType ProcessorType { get; set; }

        /// <summary>
        /// Gets or sets the additional module paths.
        /// </summary>
        IReadOnlyList<string>? AdditionalModulePaths { get; set; }

        /// <summary>
        /// Gets or sets the configuration policy.
        /// </summary>
        PowerShellConfigurationProcessorPolicy Policy { get; set; }

        /// <summary>
        /// Gets or sets the module location.
        /// </summary>
        PowerShellConfigurationProcessorLocation Location { get; set; }

        /// <summary>
        /// Gets or sets the install module path. Only used for Scope.Custom.
        /// </summary>
        string? CustomLocation { get; set; }
    }
}
