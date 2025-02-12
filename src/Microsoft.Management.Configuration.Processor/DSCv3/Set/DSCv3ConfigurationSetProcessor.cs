// -----------------------------------------------------------------------------
// <copyright file="DSCv3ConfigurationSetProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Set
{
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.Unit;
    using Microsoft.Management.Configuration.Processor.Set;
    using Microsoft.Management.Configuration.Processor.Unit;

    /// <summary>
    /// Configuration set processor.
    /// </summary>
    internal sealed partial class DSCv3ConfigurationSetProcessor : ConfigurationSetProcessorBase, IConfigurationSetProcessor
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DSCv3ConfigurationSetProcessor"/> class.
        /// </summary>
        /// <param name="configurationSet">Configuration set.</param>
        /// <param name="isLimitMode">Whether the set processor should work in limitation mode.</param>
        public DSCv3ConfigurationSetProcessor(ConfigurationSet? configurationSet, bool isLimitMode = false)
            : base(configurationSet, isLimitMode)
        {
        }

        /// <inheritdoc />
        protected override IConfigurationUnitProcessor CreateUnitProcessorInternal(ConfigurationUnit unit)
        {
            return new DSCv3ConfigurationUnitProcessor(new ConfigurationUnitInternal(unit, this.ConfigurationSet?.Path), this.IsLimitMode);
        }

        /// <inheritdoc />
        protected override IConfigurationUnitProcessorDetails? GetUnitProcessorDetailsInternal(ConfigurationUnit unit, ConfigurationUnitDetailFlags detailFlags)
        {
            // TODO: Actual implementation
            ConfigurationUnitProcessorDetails result = new ConfigurationUnitProcessorDetails() { UnitType = unit.Type };
            result.UnitDescription = "This is a test!";
            return result;
        }
    }
}
