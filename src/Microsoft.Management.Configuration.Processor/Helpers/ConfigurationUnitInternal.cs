// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitInternal.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.IO;
    using Microsoft.Management.Configuration.Processor.Constants;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Wrapper around Configuration units and its directives.
    /// Creates a normalized directives map for consumption.
    /// </summary>
    internal class ConfigurationUnitInternal
    {
        private const string ConfigRootVar = "${WinGetConfigRoot}";

        private readonly string? configurationFileRootPath = null;
        private readonly Dictionary<string, object> normalizedDirectives = new ();

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationUnitInternal"/> class.
        /// </summary>
        /// <param name="unit">Configuration unit.</param>
        /// <param name="configurationFilePath">The configuration file path.</param>
        public ConfigurationUnitInternal(
            ConfigurationUnit unit,
            string? configurationFilePath)
        {
            this.Unit = unit;
            this.InitializeDirectives();
            this.InitializeNames();

            if (!string.IsNullOrEmpty(configurationFilePath))
            {
                if (!File.Exists(configurationFilePath))
                {
                    throw new FileNotFoundException(configurationFilePath);
                }

                this.configurationFileRootPath = Path.GetDirectoryName(configurationFilePath);
            }
        }

        /// <summary>
        /// Gets the configuration unit.
        /// </summary>
        public ConfigurationUnit Unit { get; }

        /// <summary>
        /// Gets a value indicating whether the unit type should be treated as the resource name.
        /// </summary>
        public bool UnitTypeIsResourceName { get; init; } = false;

        /// <summary>
        /// Gets the resource name *only*. For example, "Resource".
        /// </summary>
        public string ResourceName { get; private set; }

        /// <summary>
        /// Gets the qualified name, which includes the module. For example, "Module/Resource".
        /// </summary>
        public string QualifiedName { get; private set; }

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
                    // For now, we just expand config root.
                    valueSet.Add(value.Key, this.ExpandConfigRoot(value.Value as string, value.Key));
                }
                else
                {
                    valueSet.Add(value);
                }
            }

            return valueSet;
        }

        private string? ExpandConfigRoot(string? value, string settingName)
        {
            if (!string.IsNullOrEmpty(value))
            {
                // TODO: since we only support one variable, this only finds and replace
                // ${WingetConfigRoot} if found in the string when the work of expanding
                // string is done it should take into account other operators like the subexpression operator $()
                if (value.Contains(ConfigRootVar, StringComparison.OrdinalIgnoreCase))
                {
                    if (string.IsNullOrEmpty(this.configurationFileRootPath))
                    {
                        throw new UnitSettingConfigRootException(this.QualifiedName, settingName);
                    }

                    return value.Replace(ConfigRootVar, this.configurationFileRootPath, StringComparison.OrdinalIgnoreCase);
                }
            }

            return value;
        }

        private void InitializeDirectives()
        {
            foreach (var directive in this.Unit.Metadata)
            {
                var normalizedKey = StringHelpers.Normalize(directive.Key);
                this.normalizedDirectives.Add(normalizedKey, directive.Value);
            }
        }

        private string ConstructQualifiedName(string? moduleName)
        {
            return $"{(moduleName == null ? string.Empty : $"{moduleName}/")}{this.ResourceName}";
        }

        [MemberNotNull(nameof(ResourceName), nameof(QualifiedName))]
        private void InitializeNames()
        {
            // Determine ResourceName, QualifiedName, and the module directive
            string unitType = this.Unit.Type;
            string? moduleDirective = this.GetDirective<string>(DirectiveConstants.Module);

            if (this.UnitTypeIsResourceName)
            {
                this.ResourceName = unitType;
                this.QualifiedName = this.ConstructQualifiedName(moduleDirective);
                return;
            }

            int unitTypeDividerPosition = unitType.IndexOf('/');

            if (unitTypeDividerPosition == unitType.Length - 1)
            {
                throw new ArgumentException($"Invalid unit Type: {unitType}");
            }

            string? moduleName;

            if (unitTypeDividerPosition == -1)
            {
                moduleName = moduleDirective;
                this.ResourceName = unitType;
                this.QualifiedName = this.ConstructQualifiedName(moduleDirective);
            }
            else
            {
                moduleName = unitType.Substring(0, unitTypeDividerPosition);
                this.ResourceName = unitType.Substring(unitTypeDividerPosition + 1);
                this.QualifiedName = unitType;
            }

            if (moduleName != null)
            {
                if (moduleDirective != null)
                {
                    if (moduleName != moduleDirective)
                    {
                        throw new ArgumentException($"Mismatched module specifiers: {moduleName} != {moduleDirective}");
                    }
                }
                else
                {
                    this.normalizedDirectives.Add(DirectiveConstants.Module, moduleName);
                }
            }
        }
    }
}
