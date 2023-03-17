// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitAndResource.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Helpers
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
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
            ConfigurationUnitInternal configurationUnitInternal,
            DscResourceInfoInternal dscResourceInfoInternal)
        {
            if (configurationUnitInternal.Unit.UnitName != dscResourceInfoInternal.Name)
            {
                throw new ArgumentException();
            }

            this.UnitInternal = configurationUnitInternal;
            this.dscResourceInfoInternal = dscResourceInfoInternal;
        }

        /// <summary>
        /// Gets or initializes the internal unit.
        /// </summary>
        public ConfigurationUnitInternal UnitInternal { get; private init; }

        /// <summary>
        /// Gets the configuration unit.
        /// </summary>
        public ConfigurationUnit Unit
        {
            get { return this.UnitInternal.Unit; }
        }

        /// <summary>
        /// Gets the directives overlay.
        /// </summary>
        public IReadOnlyDictionary<string, object>? DirectivesOverlay
        {
            get { return this.UnitInternal.DirectivesOverlay; }
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
        public string? GetDirective(string directiveName)
        {
            return this.UnitInternal.GetDirective(directiveName);
        }

        /// <summary>
        /// TODO: Implement.
        /// I am so sad because rs.SessionStateProxy.InvokeCommand.ExpandString doesn't work as I wanted.
        /// PowerShell assumes all code passed to ExpandString is trusted and we cannot assume that.
        /// </summary>
        /// <returns>ValueSet with settings.</returns>
        public ValueSet GetExpandedSettings()
        {
            return this.Unit.Settings;
        }
    }
}
