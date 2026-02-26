// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitAndResource.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.Helpers
{
    using System;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo;
    using Microsoft.PowerShell.Commands;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Contains information about the unit and the DSC resource that applies to it.
    /// </summary>
    internal class ConfigurationUnitAndResource
    {
        private readonly DscResourceInfoInternal dscResourceInfoInternal;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationUnitAndResource"/> class.
        /// </summary>
        /// <param name="configurationUnitInternal">Configuration unit internal.</param>
        /// <param name="dscResourceInfoInternal">DscResourceInfoInternal.</param>
        public ConfigurationUnitAndResource(
            ConfigurationUnitAndModule configurationUnitInternal,
            DscResourceInfoInternal dscResourceInfoInternal)
        {
            if (!configurationUnitInternal.ResourceName.Equals(dscResourceInfoInternal.Name, StringComparison.OrdinalIgnoreCase))
            {
                throw new ArgumentException();
            }

            this.UnitInternal = configurationUnitInternal;
            this.dscResourceInfoInternal = dscResourceInfoInternal;
        }

        /// <summary>
        /// Gets or initializes the internal unit.
        /// </summary>
        public ConfigurationUnitAndModule UnitInternal { get; private init; }

        /// <summary>
        /// Gets the configuration unit.
        /// </summary>
        public ConfigurationUnit Unit
        {
            get { return this.UnitInternal.Unit; }
        }

        /// <summary>
        /// Gets the DSC resource name.
        /// </summary>
        /// <returns>DSC resource name.</returns>
        public string ResourceName
        {
            get { return this.dscResourceInfoInternal.Name; }
        }

        /// <summary>
        /// Gets the module specification.
        /// </summary>
        public ModuleSpecification? Module
        {
            get { return this.UnitInternal.Module; }
        }

        /// <summary>
        /// Gets a directive if exits.
        /// </summary>
        /// <param name="directiveName">Name of directive.</param>
        /// <returns>The value of the directive. Null if doesn't exist.</returns>
        /// <typeparam name="TType">Directive type value.</typeparam>
        public TType? GetDirective<TType>(string directiveName)
            where TType : class
        {
            return this.UnitInternal.GetDirective<TType>(directiveName);
        }

        /// <summary>
        /// Gets a directive bool value.
        /// </summary>
        /// <param name="directiveName">Name of directive.</param>
        /// <returns>The value of the directive. False if not set.</returns>
        public bool? GetDirective(string directiveName)
        {
            return this.UnitInternal.GetDirective(directiveName);
        }

        /// <summary>
        /// Gets the settings of the unit.
        /// </summary>
        /// <returns>ValueSet with settings.</returns>
        public ValueSet GetSettings()
        {
            return this.UnitInternal.GetExpandedSettings();
        }
    }
}
