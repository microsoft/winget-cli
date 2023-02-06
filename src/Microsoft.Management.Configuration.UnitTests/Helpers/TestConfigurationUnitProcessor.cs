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
        private ConfigurationUnit unit;
        private IReadOnlyDictionary<string, object> directivesOverlay;

        /// <summary>
        /// Initializes a new instance of the <see cref="TestConfigurationUnitProcessor"/> class.
        /// </summary>
        /// <param name="unit">The unit.</param>
        /// <param name="directivesOverlay">The directives overlay.</param>
        internal TestConfigurationUnitProcessor(ConfigurationUnit unit, IReadOnlyDictionary<string, object> directivesOverlay)
        {
            this.unit = unit;
            this.directivesOverlay = directivesOverlay;
        }

        public IReadOnlyDictionary<string, object> DirectivesOverlay => throw new NotImplementedException();

        public ConfigurationUnit Unit => throw new NotImplementedException();

        public ApplySettingsResult ApplySettings()
        {
            throw new NotImplementedException();
        }

        public GetSettingsResult GetSettings()
        {
            throw new NotImplementedException();
        }

        public TestSettingsResult TestSettings()
        {
            throw new NotImplementedException();
        }
    }
}
