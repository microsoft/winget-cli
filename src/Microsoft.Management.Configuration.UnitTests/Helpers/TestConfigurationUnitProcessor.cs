// -----------------------------------------------------------------------------
// <copyright file="TestConfigurationUnitProcessor.cs" company="Microsoft Corporation">
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
    internal class TestConfigurationUnitProcessor : IConfigurationUnitProcessor
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="TestConfigurationUnitProcessor"/> class.
        /// </summary>
        /// <param name="unit">The unit.</param>
        internal TestConfigurationUnitProcessor(ConfigurationUnit unit)
        {
            this.Unit = unit;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TestConfigurationUnitProcessor"/> class.
        /// </summary>
        /// <param name="unit">The unit.</param>
        /// <param name="directivesOverlay">The directives overlay.</param>
        internal TestConfigurationUnitProcessor(ConfigurationUnit unit, IReadOnlyDictionary<string, object> directivesOverlay)
        {
            this.Unit = unit;
            this.DirectivesOverlay = directivesOverlay;
        }

        /// <summary>
        /// The delegate for ApplySettings.
        /// </summary>
        /// <returns>The result.</returns>
        internal delegate ApplySettingsResult ApplySettingsDelegateType();

        /// <summary>
        /// The delegate for GetSettings.
        /// </summary>
        /// <returns>The result.</returns>
        internal delegate GetSettingsResult GetSettingsDelegateType();

        /// <summary>
        /// The delegate for TestSettings.
        /// </summary>
        /// <returns>The result.</returns>
        internal delegate TestSettingsResult TestSettingsDelegateType();

        /// <summary>
        /// Gets or sets the directives overlay.
        /// </summary>
        public IReadOnlyDictionary<string, object>? DirectivesOverlay { get; set; }

        /// <summary>
        /// Gets the configuration unit.
        /// </summary>
        public ConfigurationUnit Unit { get; private set; }

        /// <summary>
        /// Gets or sets the delegate object for ApplySettings.
        /// </summary>
        internal ApplySettingsDelegateType? ApplySettingsDelegate { get; set; }

        /// <summary>
        /// Gets the number of times ApplySettings is called.
        /// </summary>
        internal int ApplySettingsCalls { get; private set; } = 0;

        /// <summary>
        /// Gets or sets the delegate object for GetSettings.
        /// </summary>
        internal GetSettingsDelegateType? GetSettingsDelegate { get; set; }

        /// <summary>
        /// Gets the number of times GetSettings is called.
        /// </summary>
        internal int GetSettingsCalls { get; private set; } = 0;

        /// <summary>
        /// Gets or sets the delegate object for TestSettings.
        /// </summary>
        internal TestSettingsDelegateType? TestSettingsDelegate { get; set; }

        /// <summary>
        /// Gets the number of times TestSettings is called.
        /// </summary>
        internal int TestSettingsCalls { get; private set; } = 0;

        /// <summary>
        /// Calls the ApplySettingsDelegate if one is provided; returns success if not.
        /// </summary>
        /// <returns>The result.</returns>
        public ApplySettingsResult ApplySettings()
        {
            ++this.ApplySettingsCalls;
            if (this.ApplySettingsDelegate != null)
            {
                return this.ApplySettingsDelegate();
            }
            else
            {
                return new ApplySettingsResult();
            }
        }

        /// <summary>
        /// Calls the GetSettingsDelegate if one is provided; returns success if not (with no settings values).
        /// </summary>
        /// <returns>The result.</returns>
        public GetSettingsResult GetSettings()
        {
            ++this.GetSettingsCalls;
            if (this.GetSettingsDelegate != null)
            {
                return this.GetSettingsDelegate();
            }
            else
            {
                return new GetSettingsResult();
            }
        }

        /// <summary>
        /// Calls the TestSettingsDelegate if one is provided; returns success if not (with a positive test result).
        /// </summary>
        /// <returns>The result.</returns>
        public TestSettingsResult TestSettings()
        {
            ++this.TestSettingsCalls;
            if (this.TestSettingsDelegate != null)
            {
                return this.TestSettingsDelegate();
            }
            else
            {
                return new TestSettingsResult { TestResult = ConfigurationTestResult.Positive };
            }
        }
    }
}
