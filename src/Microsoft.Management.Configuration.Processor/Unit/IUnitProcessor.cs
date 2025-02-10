// -----------------------------------------------------------------------------
// <copyright file="IUnitProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Internal
{
    using Windows.Foundation.Collections;

    /// <summary>
    /// The interface used by ConfigurationUnitProcessor to interact with a specific implementation.
    /// </summary>
    internal interface IUnitProcessor
    {
        /// <summary>
        /// Gets the configuration unit.
        /// </summary>
        public ConfigurationUnit Unit { get; }

        /// <summary>
        /// Gets the name to use for logging purposes.
        /// </summary>
        public string UnitNameForLogging { get; }

        /// <summary>
        /// Gets the current settings.
        /// </summary>
        /// <returns>The retrieved settings.</returns>
        public ValueSet GetSettings();

        /// <summary>
        /// Tests the current settings.
        /// </summary>
        /// <returns>A boolean indicating whether the settings are in the desired state.</returns>
        public bool TestSettings();
    }
}
