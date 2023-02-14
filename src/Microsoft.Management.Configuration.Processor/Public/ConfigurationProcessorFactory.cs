// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    using System;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.DscModule;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.Public;
    using Microsoft.Management.Configuration.Processor.Runspaces;
    using Microsoft.Management.Configuration.Processor.Set;

    /// <summary>
    /// ConfigurationProcessorFactory implementation. Does not support out-of-proc.
    /// </summary>
    public sealed class ConfigurationProcessorFactory : IConfigurationProcessorFactory
    {
        private readonly ConfigurationProcessorType type;
        private readonly IConfigurationProcessorFactoryProperties? properties;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationProcessorFactory"/> class.
        /// </summary>
        /// <param name="type">Type.</param>
        /// <param name="properties">Properties.</param>
        public ConfigurationProcessorFactory(ConfigurationProcessorType type, IConfigurationProcessorFactoryProperties? properties)
        {
            this.type = type;
            this.properties = properties;
        }

        /// <summary>
        /// Gets the configuration unit processor details for the given unit.
        /// </summary>
        /// <param name="set">Configuration Set.</param>
        /// <returns>Configuration set processor.</returns>
        public IConfigurationSetProcessor CreateSetProcessor(ConfigurationSet set)
        {
            var dscModule = this.CreateDscModule();
            var processorEnvironment = this.CreateProcessorEnvironment(dscModule);

            if (this.properties is not null)
            {
                var additionalPsModulePaths = this.properties.AdditionalModulePaths;
                if (additionalPsModulePaths is not null)
                {
                    processorEnvironment.PrependPSModulePaths(additionalPsModulePaths);
                }
            }

            return new ConfigurationSetProcessor(processorEnvironment, set);
        }

        /// <summary>
        /// Creates the IDscModule implementation for the processor type.
        /// </summary>
        /// <returns>DscModule.</returns>>
        private IDscModule CreateDscModule()
        {
            return this.type switch
            {
                ConfigurationProcessorType.Default => new DscModuleV2(customModule: false),
                ConfigurationProcessorType.Hosted => new DscModuleV2(customModule: true),
                _ => throw new NotImplementedException()
            };
        }

        private IProcessorEnvironment CreateProcessorEnvironment(IDscModule module)
        {
            return this.type switch
            {
                ConfigurationProcessorType.Default => new DefaultEnvironment(module),
                ConfigurationProcessorType.Hosted => new HostedEnvironment(module),
                _ => throw new NotImplementedException()
            };
        }
    }
}