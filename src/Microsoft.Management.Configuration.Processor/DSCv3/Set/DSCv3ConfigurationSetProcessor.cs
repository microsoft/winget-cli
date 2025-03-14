// -----------------------------------------------------------------------------
// <copyright file="DSCv3ConfigurationSetProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Set
{
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.Processor.DSCv3.Helpers;
    using Microsoft.Management.Configuration.Processor.DSCv3.Unit;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.Set;

    /// <summary>
    /// Configuration set processor.
    /// </summary>
    internal sealed partial class DSCv3ConfigurationSetProcessor : ConfigurationSetProcessorBase, IConfigurationSetProcessor
    {
        private readonly ProcessorSettings processorSettings;
        private Dictionary<string, ResourceDetails> resourceDetailsDictionary = new ();

        /// <summary>
        /// Initializes a new instance of the <see cref="DSCv3ConfigurationSetProcessor"/> class.
        /// </summary>
        /// <param name="processorSettings">The processor settings to use.</param>
        /// <param name="configurationSet">Configuration set.</param>
        /// <param name="isLimitMode">Whether the set processor should work in limitation mode.</param>
        public DSCv3ConfigurationSetProcessor(ProcessorSettings processorSettings, ConfigurationSet? configurationSet, bool isLimitMode = false)
            : base(configurationSet, isLimitMode)
        {
            this.processorSettings = processorSettings;
        }

        /// <inheritdoc />
        protected override IConfigurationUnitProcessor CreateUnitProcessorInternal(ConfigurationUnit unit)
        {
            ConfigurationUnitInternal configurationUnitInternal = new ConfigurationUnitInternal(unit, this.ConfigurationSet?.Path);
            this.OnDiagnostics(DiagnosticLevel.Verbose, $"Creating unit processor for: {configurationUnitInternal.QualifiedName}...");

            ResourceDetails? resourceDetails = this.GetResourceDetails(configurationUnitInternal, ConfigurationUnitDetailFlags.Local);
            if (resourceDetails == null)
            {
                this.OnDiagnostics(DiagnosticLevel.Verbose, $"Resource not found: {configurationUnitInternal.QualifiedName}");
                throw new Exceptions.FindDscResourceNotFoundException(configurationUnitInternal.QualifiedName, null);
            }

            return new DSCv3ConfigurationUnitProcessor(this.processorSettings, configurationUnitInternal, this.IsLimitMode) { SetProcessorFactory = this.SetProcessorFactory };
        }

        /// <inheritdoc />
        protected override IConfigurationUnitProcessorDetails? GetUnitProcessorDetailsInternal(ConfigurationUnit unit, ConfigurationUnitDetailFlags detailFlags)
        {
            ConfigurationUnitInternal configurationUnitInternal = new ConfigurationUnitInternal(unit, this.ConfigurationSet?.Path);
            this.OnDiagnostics(DiagnosticLevel.Verbose, $"Getting resource details [{detailFlags}] for: {configurationUnitInternal.QualifiedName}...");

            ResourceDetails? resourceDetails = this.GetResourceDetails(configurationUnitInternal, detailFlags);
            if (resourceDetails == null)
            {
                this.OnDiagnostics(DiagnosticLevel.Verbose, $"Resource not found: {configurationUnitInternal.QualifiedName}");
                return null;
            }

            return resourceDetails.GetConfigurationUnitProcessorDetails();
        }

        private ResourceDetails? GetResourceDetails(ConfigurationUnitInternal configurationUnitInternal, ConfigurationUnitDetailFlags detailFlags)
        {
            ResourceDetails? result = null;
            bool inDictionary = false;

            lock (this.resourceDetailsDictionary)
            {
                inDictionary = this.resourceDetailsDictionary.TryGetValue(configurationUnitInternal.QualifiedName, out result);
            }

            if (result == null)
            {
                result = new ResourceDetails(configurationUnitInternal);
            }

            result.EnsureDetails(this.processorSettings, detailFlags);

            if (result.Exists)
            {
                if (!inDictionary)
                {
                    lock (this.resourceDetailsDictionary)
                    {
                        this.resourceDetailsDictionary.Add(configurationUnitInternal.QualifiedName, result);
                    }
                }

                return result;
            }
            else
            {
                return null;
            }
        }
    }
}
