// -----------------------------------------------------------------------------
// <copyright file="ConfigurationSetProcessorBase.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Set
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.CompilerServices;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.Processor.Factory;
    using Microsoft.Management.Configuration.Processor.Unit;

    /// <summary>
    /// IConfigurationSetProcessor base implementation.
    /// </summary>
    internal abstract partial class ConfigurationSetProcessorBase
    {
        private readonly ConfigurationSet? configurationSet;
        private List<ConfigurationUnit> limitUnitList = new List<ConfigurationUnit>();

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationSetProcessorBase"/> class.
        /// </summary>
        /// <param name="configurationSet">Configuration set.</param>
        /// <param name="isLimitMode">Whether the set processor should work in limitation mode.</param>
        public ConfigurationSetProcessorBase(ConfigurationSet? configurationSet, bool isLimitMode)
        {
            this.configurationSet = configurationSet;
            this.IsLimitMode = isLimitMode;

            // In limit mode, configurationSet is the limitation set to be used. It cannot be null.
            if (this.IsLimitMode)
            {
                if (this.configurationSet == null)
                {
                    throw new ArgumentNullException(nameof(configurationSet), "configurationSet is required in limit mode.");
                }

                foreach (var unit in this.configurationSet.Units)
                {
                    this.limitUnitList.Add(unit);
                }
            }
        }

        /// <summary>
        /// Gets or initializes the set processor factory.
        /// </summary>
        internal ConfigurationSetProcessorFactoryBase? SetProcessorFactory { get; init; }

        /// <summary>
        /// Gets a value indicating whether the set processor is running in limit mode.
        /// </summary>
        internal bool IsLimitMode { get; private set; }

        /// <summary>
        /// Gets the configuration set for this processor.
        /// </summary>
        protected ConfigurationSet? ConfigurationSet
        {
            get { return this.configurationSet; }
        }

        /// <summary>
        /// Creates a configuration unit processor for the given unit.
        /// </summary>
        /// <param name="incomingUnit">Configuration unit.</param>
        /// <returns>A configuration unit processor.</returns>
        public IConfigurationUnitProcessor CreateUnitProcessor(ConfigurationUnit incomingUnit)
        {
            try
            {
                this.OnDiagnostics(DiagnosticLevel.Informational, $"GetUnitProcessorDetails is running in limit mode: {this.IsLimitMode}.");

                // CreateUnitProcessor can only be called once on each configuration unit in limit mode.
                var unit = this.GetConfigurationUnit(incomingUnit, true);

                IConfigurationUnitProcessor result = this.CreateUnitProcessorInternal(unit);

                this.OnDiagnostics(DiagnosticLevel.Verbose, "... done creating unit processor.");

                return result;
            }
            catch (Exception ex)
            {
                this.OnDiagnostics(DiagnosticLevel.Error, ex.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the configuration unit processor details for the given unit.
        /// </summary>
        /// <param name="incomingUnit">Configuration unit.</param>
        /// <param name="detailFlags">Detail flags.</param>
        /// <returns>Configuration unit processor details.</returns>
        public IConfigurationUnitProcessorDetails? GetUnitProcessorDetails(ConfigurationUnit incomingUnit, ConfigurationUnitDetailFlags detailFlags)
        {
            try
            {
                this.OnDiagnostics(DiagnosticLevel.Informational, $"GetUnitProcessorDetails is running in limit mode: {this.IsLimitMode}.");

                // GetUnitProcessorDetails can be invoked multiple times on each configuration unit in limit mode.
                var unit = this.GetConfigurationUnit(incomingUnit);

                return this.GetUnitProcessorDetailsInternal(unit, detailFlags);
            }
            catch (Exception ex)
            {
                this.OnDiagnostics(DiagnosticLevel.Error, ex.ToString());
                throw;
            }
        }

        /// <summary>
        /// Creates a configuration unit processor for the given unit.
        /// </summary>
        /// <param name="unit">Configuration unit.</param>
        /// <returns>A configuration unit processor.</returns>
        protected abstract IConfigurationUnitProcessor CreateUnitProcessorInternal(ConfigurationUnit unit);

        /// <summary>
        /// Gets the configuration unit processor details for the given unit.
        /// </summary>
        /// <param name="unit">Configuration unit.</param>
        /// <param name="detailFlags">Detail flags.</param>
        /// <returns>Configuration unit processor details.</returns>
        protected abstract IConfigurationUnitProcessorDetails? GetUnitProcessorDetailsInternal(ConfigurationUnit unit, ConfigurationUnitDetailFlags detailFlags);

        /// <summary>
        /// Sends diagnostics to factory.
        /// </summary>
        /// <param name="level">The level of this diagnostic message.</param>
        /// <param name="message">The diagnostic message.</param>
        protected void OnDiagnostics(DiagnosticLevel level, string message)
        {
            this.SetProcessorFactory?.OnDiagnostics(level, message);
        }

        private static bool ConfigurationUnitEquals(ConfigurationUnit first, ConfigurationUnit second)
        {
            var firstIdentifier = first.Identifier;
            var firstIntent = first.Intent;
            var firstType = first.Type;
            var secondIdentifier = second.Identifier;
            var secondType = second.Type;
            var secondIntent = second.Intent;

            if (firstIdentifier != secondIdentifier ||
                firstType != secondType ||
                firstIntent != secondIntent)
            {
                return false;
            }

            var firstEnvironment = first.Environment;
            var secondEnvironment = second.Environment;
            if (firstEnvironment.Context != secondEnvironment.Context ||
                firstEnvironment.ProcessorIdentifier != secondEnvironment.ProcessorIdentifier ||
                !firstEnvironment.ProcessorProperties.ContentEquals(secondEnvironment.ProcessorProperties))
            {
                return false;
            }

            if (!first.Settings.ContentEquals(second.Settings))
            {
                return false;
            }

            if (!first.Metadata.ContentEquals(second.Metadata))
            {
                return false;
            }

            // Note: Consider group units logic when group units are supported.
            return true;
        }

        [MethodImpl(MethodImplOptions.Synchronized)]
        private ConfigurationUnit GetConfigurationUnit(ConfigurationUnit incomingUnit, bool useLimitList = false)
        {
            if (this.IsLimitMode)
            {
                if (this.configurationSet == null)
                {
                    throw new InvalidOperationException("Configuration set should not be null in limit mode.");
                }

                var unitList = useLimitList ? this.limitUnitList : this.configurationSet.Units;

                for (int i = 0; i < unitList.Count; i++)
                {
                    var unit = unitList[i];
                    if (ConfigurationUnitEquals(incomingUnit, unit))
                    {
                        if (useLimitList)
                        {
                            this.limitUnitList.RemoveAt(i);
                        }

                        return unit;
                    }

                    // Note: Consider group units logic when group units are supported.
                }

                this.OnDiagnostics(DiagnosticLevel.Error, "Configuration unit not found in limit mode.");
                throw new InvalidOperationException("Configuration unit not found in limit mode.");
            }
            else
            {
                return incomingUnit;
            }
        }
    }
}
