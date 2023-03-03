// -----------------------------------------------------------------------------
// <copyright file="ConfigurationSetProcessorFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.Set;

    /// <summary>
    /// ConfigurationProcessorFactory implementation. Does not support out-of-proc.
    /// TODO: change to IConfigurationSetProcessorFactory.
    /// </summary>
    public sealed class ConfigurationSetProcessorFactory : IConfigurationProcessorFactory
    {
        private readonly ConfigurationProcessorType type;
        private readonly IConfigurationProcessorFactoryProperties? properties;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationSetProcessorFactory"/> class.
        /// </summary>
        /// <param name="type">Type.</param>
        /// <param name="properties">Properties.</param>
        public ConfigurationSetProcessorFactory(ConfigurationProcessorType type, IConfigurationProcessorFactoryProperties? properties)
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
            var envFactory = new ProcessorEnvironmentFactory(this.type);
            var processorEnvironment = envFactory.CreateEnvironment();
            processorEnvironment.ValidateRunspace();

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
    }
}