// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitAndModule.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.Helpers
{
    using System;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.Constants;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// Contains information about the unit and the DSC resource that applies to it.
    /// </summary>
    internal class ConfigurationUnitAndModule : ConfigurationUnitInternal
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationUnitAndModule"/> class.
        /// </summary>
        /// <param name="unit">Configuration unit.</param>
        /// <param name="configurationFilePath">The configuration file path.</param>
        public ConfigurationUnitAndModule(ConfigurationUnit unit, string? configurationFilePath)
            : base(unit, configurationFilePath)
        {
            string? moduleName = this.GetDirective<string>(DirectiveConstants.Module);
            if (string.IsNullOrEmpty(moduleName))
            {
                this.Module = null;
            }
            else
            {
                this.Module = PowerShellHelpers.CreateModuleSpecification(
                    moduleName,
                    this.GetDirective<string>(DirectiveConstants.Version),
                    this.GetDirective<string>(DirectiveConstants.MinVersion),
                    this.GetDirective<string>(DirectiveConstants.MaxVersion),
                    this.GetDirective<string>(DirectiveConstants.ModuleGuid));
            }
        }

        /// <summary>
        /// Gets the module specification.
        /// </summary>
        public ModuleSpecification? Module { get; }
    }
}
