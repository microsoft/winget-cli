// -----------------------------------------------------------------------------
// <copyright file="DSCv3ConfigurationUnitProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Unit
{
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.DSCv3.Helpers;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.Unit;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Provides access to a specific configuration unit within the runtime.
    /// </summary>
    internal sealed partial class DSCv3ConfigurationUnitProcessor : ConfigurationUnitProcessorBase, IConfigurationUnitProcessor
    {
        private readonly ProcessorSettings processorSettings;

        /// <summary>
        /// Initializes a new instance of the <see cref="DSCv3ConfigurationUnitProcessor"/> class.
        /// </summary>
        /// <param name="processorSettings">The processor settings to use.</param>
        /// <param name="unitInternal">Internal unit.</param>
        /// <param name="isLimitMode">Whether it is under limit mode.</param>
        internal DSCv3ConfigurationUnitProcessor(ProcessorSettings processorSettings, ConfigurationUnitInternal unitInternal, bool isLimitMode = false)
            : base(unitInternal, isLimitMode)
        {
            this.processorSettings = processorSettings;
        }

        /// <inheritdoc />
        protected override ValueSet GetSettingsInternal()
        {
            // TODO: Actual implementation
            var result = new ValueSet();
            result.Add("Test", "Value");
            return result;
        }

        /// <inheritdoc />
        protected override bool TestSettingsInternal()
        {
            // TODO: Actual implementation
            return false;
        }

        /// <inheritdoc />
        protected override bool ApplySettingsInternal()
        {
            // TODO: Actual implementation
            return false;
        }
    }
}
