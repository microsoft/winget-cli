// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitInternal.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Helpers
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.Processor.Constants;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.PowerShell.Commands;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Wrapper around Configuration units and its directives. Creates a normalized directives map
    /// for consumption.
    /// </summary>
    internal class ConfigurationUnitInternal
    {
        private static string configRoot = "$WinGetConfigRoot";

        private readonly string origin;
        private readonly Dictionary<string, object> normalizedDirectives = new ();

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationUnitInternal"/> class.
        /// </summary>
        /// <param name="unit">Configuration unit.</param>
        /// <param name="origin">The origin of the unit.</param>
        /// <param name="directivesOverlay">Directives overlay.</param>
        public ConfigurationUnitInternal(
            ConfigurationUnit unit,
            string origin,
            IReadOnlyDictionary<string, object>? directivesOverlay = null)
        {
            this.Unit = unit;
            this.origin = origin;
            this.DirectivesOverlay = directivesOverlay;
            this.InitializeDirectives();

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
        /// Gets the configuration unit.
        /// </summary>
        public ConfigurationUnit Unit { get; }

        /// <summary>
        /// Gets the directives overlay.
        /// </summary>
        public IReadOnlyDictionary<string, object>? DirectivesOverlay { get; }

        /// <summary>
        /// Gets the module specification.
        /// </summary>
        public ModuleSpecification? Module { get; }

        /// <summary>
        /// Creates a string that identifies this unit for diagnostics.
        /// </summary>
        /// <returns>The string that identifies this unit for diagnostics.</returns>
        public string ToIdentifyingString()
        {
            return $"{this.Unit.UnitName} [{this.Module?.ToString() ?? "<no module>"}]";
        }

        /// <summary>
        /// Gets the directive value from the unit taking into account the directives overlay.
        /// </summary>
        /// <param name="directiveName">Directive name.</param>
        /// <returns>Value of directive, null if not found.</returns>
        /// <typeparam name="TType">Directive type value.</typeparam>
        public TType? GetDirective<TType>(string directiveName)
            where TType : class
        {
            var normalizedDirectiveName = StringHelpers.Normalize(directiveName);
            if (this.normalizedDirectives.TryGetValue(normalizedDirectiveName, out object? value))
            {
                return value as TType;
            }

            return null;
        }

        /// <summary>
        /// Gets the bool value of a directive from the unit taking into account the directives overlay.
        /// </summary>
        /// <param name="directiveName">Directive name.</param>
        /// <returns>Value of directive, false if not found.</returns>
        public bool? GetDirective(string directiveName)
        {
            var normalizedDirectiveName = StringHelpers.Normalize(directiveName);
            if (this.normalizedDirectives.TryGetValue(normalizedDirectiveName, out object? value))
            {
                return value as bool?;
            }

            return null;
        }

        /// <summary>
        /// Gets the semantic version, if any.
        /// </summary>
        /// <returns>SemanticVersion, null if not specified.</returns>
        public SemanticVersion? GetSemanticVersion()
        {
            string? semanticVersion = this.GetDirective<string>(DirectiveConstants.Version);
            if (!string.IsNullOrWhiteSpace(semanticVersion))
            {
                return new SemanticVersion(semanticVersion);
            }

            return null;
        }

        /// <summary>
        /// Gets the semantic min version, if any.
        /// </summary>
        /// <returns>SemanticVersion, null if not specified.</returns>
        public SemanticVersion? GetSemanticMinVersion()
        {
            string? semanticVersion = this.GetDirective<string>(DirectiveConstants.MinVersion);
            if (!string.IsNullOrWhiteSpace(semanticVersion))
            {
                return new SemanticVersion(semanticVersion);
            }

            return null;
        }

        /// <summary>
        /// Gets the semantic max version, if any.
        /// </summary>
        /// <returns>SemanticVersion, null if not specified.</returns>
        public SemanticVersion? GetSemanticMaxVersion()
        {
            string? semanticVersion = this.GetDirective<string>(DirectiveConstants.MaxVersion);
            if (!string.IsNullOrWhiteSpace(semanticVersion))
            {
                return new SemanticVersion(semanticVersion);
            }

            return null;
        }

        /// <summary>
        /// TODO: Implement for more variables.
        /// I am so sad because rs.SessionStateProxy.InvokeCommand.ExpandString doesn't work as I wanted.
        /// PowerShell assumes all code passed to ExpandString is trusted and we cannot assume that.
        /// </summary>
        /// <returns>ValueSet with settings.</returns>
        public ValueSet GetExpandedSettings()
        {
            var valueSet = new ValueSet();
            foreach (var value in this.Unit.Settings)
            {
                if (value.Value is string)
                {
                    // For now, we just expand origin.
                    valueSet.Add(value.Key, this.ExpandOrigin(value.Value as string, value.Key));
                }
                else
                {
                    valueSet.Add(value);
                }
            }

            return valueSet;
        }

        private string? ExpandOrigin(string? value, string settingName)
        {
            if (!string.IsNullOrEmpty(value))
            {
                // TODO: since we only support one variable, this only finds and replace
                // $WingetConfigRoot if found in the string when the work of expanding
                // string is done it should take into account other operators like the subexpression operator $()
                if (value.Contains(configRoot, StringComparison.OrdinalIgnoreCase))
                {
                    if (string.IsNullOrEmpty(this.origin))
                    {
                        throw new UnitSettingConfigRootException(this.Unit.UnitName, settingName);
                    }

                    return value.Replace(configRoot, this.origin, StringComparison.OrdinalIgnoreCase);
                }
            }

            return value;
        }

        private void InitializeDirectives()
        {
            // Overlay directives have precedence.
            if (this.DirectivesOverlay is not null)
            {
                foreach (var directive in this.DirectivesOverlay)
                {
                    var normalizedKey = StringHelpers.Normalize(directive.Key);
                    this.normalizedDirectives.Add(normalizedKey, directive.Value);
                }
            }

            foreach (var directive in this.Unit.Directives)
            {
                var normalizedKey = StringHelpers.Normalize(directive.Key);
                _ = this.normalizedDirectives.TryAdd(normalizedKey, directive.Value);
            }
        }
    }
}
