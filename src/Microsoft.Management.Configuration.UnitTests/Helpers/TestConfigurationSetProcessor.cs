// -----------------------------------------------------------------------------
// <copyright file="TestConfigurationSetProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// A test implementation of IConfigurationSetProcessor.
    /// </summary>
    internal partial class TestConfigurationSetProcessor : IConfigurationSetProcessor
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="TestConfigurationSetProcessor"/> class.
        /// </summary>
        /// <param name="set">The set that this processor is for.</param>
        internal TestConfigurationSetProcessor(ConfigurationSet? set)
        {
            this.Set = set;
        }

        /// <summary>
        /// Gets or sets the processors to be used by this factory.
        /// </summary>
        internal Dictionary<ConfigurationUnit, TestConfigurationUnitProcessor> Processors { get; set; } =
            new Dictionary<ConfigurationUnit, TestConfigurationUnitProcessor>();

        /// <summary>
        /// Gets or sets the details to be used by this factory.
        /// </summary>
        internal Dictionary<ConfigurationUnit, TestConfigurationUnitProcessorDetails> Details { get; set; } =
            new Dictionary<ConfigurationUnit, TestConfigurationUnitProcessorDetails>();

        /// <summary>
        /// Gets or sets the exceptions to be used by this factory.
        /// </summary>
        internal Dictionary<ConfigurationUnit, Exception> Exceptions { get; set; } =
            new Dictionary<ConfigurationUnit, Exception>();

        /// <summary>
        /// Gets or sets a value indicating whether the default unit processors for groups will enable group processing.
        /// </summary>
        internal bool EnableDefaultGroupProcessorCreation { get; set; } = false;

        /// <summary>
        /// Gets the ConfigurationSet that this processor targets.
        /// </summary>
        protected ConfigurationSet? Set { get; private set; }

        /// <summary>
        /// Creates a new unit processor for the given unit.
        /// </summary>
        /// <param name="unit">The unit.</param>
        /// <returns>The configuration unit processor.</returns>
        public IConfigurationUnitProcessor CreateUnitProcessor(ConfigurationUnit unit)
        {
            if (this.Exceptions.ContainsKey(unit))
            {
                throw this.Exceptions[unit];
            }

            if (!this.Processors.ContainsKey(unit))
            {
                if (this.EnableDefaultGroupProcessorCreation && unit.IsGroup)
                {
                    this.Processors.Add(unit, new TestConfigurationUnitGroupProcessor(unit));
                }
                else
                {
                    this.Processors.Add(unit, new TestConfigurationUnitProcessor(unit));
                }
            }

            return this.Processors[unit];
        }

        /// <summary>
        /// Gets the unit processor details for the given unit.
        /// </summary>
        /// <param name="unit">The unit.</param>
        /// <param name="detailFlags">The detail flags.</param>
        /// <returns>The details requested.</returns>
        public IConfigurationUnitProcessorDetails GetUnitProcessorDetails(ConfigurationUnit unit, ConfigurationUnitDetailFlags detailFlags)
        {
            if (this.Exceptions.ContainsKey(unit))
            {
                throw this.Exceptions[unit];
            }

            if (!this.Details.ContainsKey(unit))
            {
                this.Details.Add(unit, new TestConfigurationUnitProcessorDetails(unit, detailFlags));
            }

            return this.Details[unit];
        }

        /// <summary>
        /// Creates a new test processor for the given unit.
        /// </summary>
        /// <param name="unit">The unit.</param>
        /// <returns>The configuration unit processor.</returns>
        internal TestConfigurationUnitProcessor CreateTestProcessor(ConfigurationUnit unit)
        {
            this.Processors[unit] = new TestConfigurationUnitProcessor(unit);
            return this.Processors[unit];
        }

        /// <summary>
        /// Creates a new test group processor for the given unit.
        /// </summary>
        /// <param name="unit">The unit.</param>
        /// <returns>A new TestConfigurationUnitGroupProcessor for the unit.</returns>
        internal TestConfigurationUnitGroupProcessor CreateTestGroupProcessor(ConfigurationUnit unit)
        {
            TestConfigurationUnitGroupProcessor result = new (unit);
            this.Processors[unit] = result;
            return result;
        }

        /// <summary>
        /// Creates a new test processor that supports GetAllSettings for the given unit.
        /// </summary>
        /// <param name="unit">The unit.</param>
        /// <returns>The configuration unit processor.</returns>
        internal TestGetAllSettingsConfigurationUnitProcessor CreateGetAllSettingsTestProcessor(ConfigurationUnit unit)
        {
            var processor = new TestGetAllSettingsConfigurationUnitProcessor(unit);
            this.Processors[unit] = processor;
            return processor;
        }

        /// <summary>
        /// Creates a new unit processor details for the given unit.
        /// </summary>
        /// <param name="unit">The unit.</param>
        /// <param name="detailFlags">The detail flags.</param>
        /// <returns>The details requested.</returns>
        internal TestConfigurationUnitProcessorDetails CreateUnitDetails(ConfigurationUnit unit, ConfigurationUnitDetailFlags detailFlags)
        {
            this.Details[unit] = new TestConfigurationUnitProcessorDetails(unit, detailFlags);
            return this.Details[unit];
        }
    }
}
