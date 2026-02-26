// -----------------------------------------------------------------------------
// <copyright file="PowerShellConfigurationUnitProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.Unit
{
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.Unit;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Provides access to a specific configuration unit within the runtime.
    /// </summary>
    internal sealed partial class PowerShellConfigurationUnitProcessor : ConfigurationUnitProcessorBase, IConfigurationUnitProcessor
    {
        private readonly IProcessorEnvironment processorEnvironment;
        private readonly ConfigurationUnitAndResource unitResource;

        /// <summary>
        /// Initializes a new instance of the <see cref="PowerShellConfigurationUnitProcessor"/> class.
        /// </summary>
        /// <param name="processorEnvironment">Processor environment.</param>
        /// <param name="unitResource">UnitResource.</param>
        /// <param name="isLimitMode">Whether it is under limit mode.</param>
        internal PowerShellConfigurationUnitProcessor(IProcessorEnvironment processorEnvironment, ConfigurationUnitAndResource unitResource, bool isLimitMode = false)
            : base(unitResource.UnitInternal, isLimitMode)
        {
            this.processorEnvironment = processorEnvironment;
            this.unitResource = unitResource;
        }

        /// <inheritdoc />
        protected override ValueSet GetSettingsInternal()
        {
            return this.processorEnvironment.InvokeGetResource(
                this.unitResource.GetSettings(),
                this.unitResource.ResourceName,
                this.unitResource.Module);
        }

        /// <inheritdoc />
        protected override bool TestSettingsInternal()
        {
            return this.processorEnvironment.InvokeTestResource(
                this.unitResource.GetSettings(),
                this.unitResource.ResourceName,
                this.unitResource.Module);
        }

        /// <inheritdoc />
        protected override bool ApplySettingsInternal()
        {
            return this.processorEnvironment.InvokeSetResource(
                this.unitResource.GetSettings(),
                this.unitResource.ResourceName,
                this.unitResource.Module);
        }
    }
}
