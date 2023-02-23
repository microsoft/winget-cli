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
        /// Delegate type for CreateSetProcessor.
        /// </summary>
        /// <param name="factory">The TestConfigurationProcessorFactory that is calling this function.</param>
        /// <param name="configurationSet">The set.</param>
        /// <returns>A new TestConfigurationSetProcessor for the set.</returns>
        internal delegate IConfigurationSetProcessor CreateSetProcessorDelegateType(TestConfigurationProcessorFactory factory, ConfigurationSet configurationSet);

        /// <summary>
        /// Gets or sets the processor used when the incoming configuration set is null.
        /// </summary>
        internal TestConfigurationSetProcessor? NullProcessor { get; set; }

        /// <summary>
        /// Gets or sets the processors to be used by this factory.
        /// </summary>
        internal Dictionary<ConfigurationSet, TestConfigurationSetProcessor> Processors { get; set; } =
            new Dictionary<ConfigurationSet, TestConfigurationSetProcessor>();

        /// <summary>
        /// Gets or sets the exception used when the incoming configuration set is null.
        /// </summary>
        internal Exception? NullException { get; set; }

        /// <summary>
        /// Gets or sets the exceptions to be used by this factory.
        /// </summary>
        internal Dictionary<ConfigurationSet, Exception> Exceptions { get; set; } =
            new Dictionary<ConfigurationSet, Exception>();

        /// <summary>
        /// Gets or sets the delegate use to replace the default CreateSetProcessor functionality.
        /// </summary>
        internal CreateSetProcessorDelegateType? CreateSetProcessorDelegate { get; set; }

        /// <summary>
        /// Creates a new TestConfigurationSetProcessor for the set.
        /// </summary>
        /// <param name="configurationSet">The set.</param>
        /// <returns>A new TestConfigurationSetProcessor for the set.</returns>
        public IConfigurationSetProcessor CreateSetProcessor(ConfigurationSet configurationSet)
        {
            if (this.CreateSetProcessorDelegate != null)
            {
                return this.CreateSetProcessorDelegate(this, configurationSet);
            }

            return this.DefaultCreateSetProcessor(configurationSet);
        }

        /// <summary>
        /// The default test implementation that creates a new TestConfigurationSetProcessor for the set.
        /// </summary>
        /// <param name="configurationSet">The set.</param>
        /// <returns>A new TestConfigurationSetProcessor for the set.</returns>
        internal IConfigurationSetProcessor DefaultCreateSetProcessor(ConfigurationSet configurationSet)
        {
            if (configurationSet == null)
            {
                if (this.NullException != null)
                {
                    throw this.NullException;
                }

                if (this.NullProcessor == null)
                {
                    this.NullProcessor = new TestConfigurationSetProcessor(null);
                }

                return this.NullProcessor;
            }

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

        /// <summary>
        /// A convenience function to create a new processor for the given set and store it in the dictionary for use in the test.
        /// </summary>
        /// <param name="configurationSet">The set.</param>
        /// <returns>A new TestConfigurationSetProcessor for the set.</returns>
        internal TestConfigurationSetProcessor CreateTestProcessor(ConfigurationSet configurationSet)
        {
            this.Processors[configurationSet] = new TestConfigurationSetProcessor(configurationSet);
            return this.Processors[configurationSet];
        }
    }
}
