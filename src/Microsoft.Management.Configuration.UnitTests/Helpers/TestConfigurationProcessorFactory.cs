// -----------------------------------------------------------------------------
// <copyright file="TestConfigurationProcessorFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// A test implementation of IConfigurationProcessorFactory.
    /// </summary>
    internal class TestConfigurationProcessorFactory : IConfigurationProcessorFactory
    {
        /// <summary>
        /// Gets or sets the processors to be used by this factory.
        /// </summary>
        internal Dictionary<ConfigurationSet, TestConfigurationSetProcessor> Processors { get; set; } =
            new Dictionary<ConfigurationSet, TestConfigurationSetProcessor>();

        /// <summary>
        /// Gets or sets the exceptions to be used by this factory.
        /// </summary>
        internal Dictionary<ConfigurationSet, Exception> Exceptions { get; set; } =
            new Dictionary<ConfigurationSet, Exception>();

        /// <summary>
        /// Creates a new TestConfigurationSetProcessor for the set.
        /// </summary>
        /// <param name="configurationSet">The set.</param>
        /// <returns>A new TestConfigurationSetProcessor for the set.</returns>
        public IConfigurationSetProcessor CreateSetProcessor(ConfigurationSet configurationSet)
        {
            if (this.Exceptions.ContainsKey(configurationSet))
            {
                throw this.Exceptions[configurationSet];
            }

            if (!this.Processors.ContainsKey(configurationSet))
            {
                this.Processors.Add(configurationSet, new TestConfigurationSetProcessor(configurationSet));
            }

            return this.Processors[configurationSet];
        }
    }
}
