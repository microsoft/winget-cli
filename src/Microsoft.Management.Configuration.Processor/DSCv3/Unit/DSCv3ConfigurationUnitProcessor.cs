// -----------------------------------------------------------------------------
// <copyright file="DSCv3ConfigurationUnitProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Unit
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.DSCv3.Helpers;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.Unit;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Provides access to a specific configuration unit within the runtime.
    /// </summary>
    internal sealed partial class DSCv3ConfigurationUnitProcessor : ConfigurationUnitProcessorBase, IConfigurationUnitProcessor, IGetAllSettingsConfigurationUnitProcessor, IGetAllUnitsConfigurationUnitProcessor, IDiagnosticsSink
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
        void IDiagnosticsSink.OnDiagnostics(DiagnosticLevel level, string message)
        {
            this.OnDiagnostics(level, message);
        }

        /// <inheritdoc />
        protected override ValueSet GetSettingsInternal()
        {
            return this.processorSettings.DSCv3.GetResourceSettings(this.UnitInternal, this).Settings;
        }

        /// <inheritdoc />
        protected override bool TestSettingsInternal()
        {
            return this.processorSettings.DSCv3.TestResource(this.UnitInternal, this).InDesiredState;
        }

        /// <inheritdoc />
        protected override bool ApplySettingsInternal()
        {
            return this.processorSettings.DSCv3.SetResourceSettings(this.UnitInternal, this).RebootRequired;
        }

        /// <inheritdoc />
        protected override IList<ValueSet>? GetAllSettingsInternal()
        {
            var exportResult = this.processorSettings.DSCv3.ExportResource(this.UnitInternal, this);

            string expectedType = this.UnitInternal.QualifiedName.ToLowerInvariant();
            List<ValueSet> result = new List<ValueSet>();

            foreach (var exportItem in exportResult)
            {
                if (exportItem.Type.ToLowerInvariant() != expectedType)
                {
                    throw new UnitPropertyUnsupportedException(typeof(IGetAllSettingsConfigurationUnitProcessor));
                }

                result.Add(exportItem.Settings);
            }

            return result;
        }

        /// <inheritdoc />
        protected override IList<ConfigurationUnit>? GetAllUnitsInternal()
        {
            var exportResult = this.processorSettings.DSCv3.ExportResource(this.UnitInternal, this);

            List<ConfigurationUnit> result = new List<ConfigurationUnit>();

            foreach (var exportItem in exportResult)
            {
                ConfigurationUnit unit = new ConfigurationUnit();

                unit.Type = exportItem.Type;
                unit.Identifier = exportItem.Name;
                unit.Settings = exportItem.Settings;
                unit.Metadata = exportItem.Metadata;
                unit.Dependencies = exportItem.Dependencies;

                result.Add(unit);
            }

            return result;
        }
    }
}
