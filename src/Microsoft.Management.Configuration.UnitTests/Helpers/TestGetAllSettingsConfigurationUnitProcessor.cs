// -----------------------------------------------------------------------------
// <copyright file="TestGetAllSettingsConfigurationUnitProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    /// <summary>
    /// A test implementation of IConfigurationProcessorFactory.
    /// </summary>
    internal partial class TestGetAllSettingsConfigurationUnitProcessor : TestConfigurationUnitProcessor, IGetAllSettingsConfigurationUnitProcessor
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="TestGetAllSettingsConfigurationUnitProcessor"/> class.
        /// </summary>
        /// <param name="unit">The unit.</param>
        internal TestGetAllSettingsConfigurationUnitProcessor(ConfigurationUnit unit)
            : base(unit)
        {
        }

        /// <summary>
        /// The delegate for GetAllSettings.
        /// </summary>
        /// <returns>The result.</returns>
        internal delegate IGetAllSettingsResult GetAllSettingsDelegateType();

        /// <summary>
        /// Gets or sets the delegate object for GetAllSettings.
        /// </summary>
        internal GetAllSettingsDelegateType? GetAllSettingsDelegate { get; set; }

        /// <summary>
        /// Gets the number of times GetAllSettings is called.
        /// </summary>
        internal int GetAllSettingsCalls { get; private set; } = 0;

        /// <summary>
        /// Calls the GetAllSettingsDelegate if one is provided; returns success if not (with no settings values).
        /// </summary>
        /// <returns>The result.</returns>
        public IGetAllSettingsResult GetAllSettings()
        {
            ++this.GetAllSettingsCalls;
            if (this.GetAllSettingsDelegate != null)
            {
                return this.GetAllSettingsDelegate();
            }
            else
            {
                return new GetAllSettingsResultInstance(this.Unit);
            }
        }
    }
}
