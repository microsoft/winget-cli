// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitInformation.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Management.Automation;
    using System.Text;
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.Extensions;
    using Microsoft.WinGet.Resources;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Helper class to construct the information messages for a unit.
    /// This must match or be as close as possible to winget's OutputConfigurationUnitInformation.
    /// </summary>
    internal class ConfigurationUnitInformation
    {
        private const string Description = "description";
        private const string Module = "module";
        private const string TreatAsArray = "treatAsArray";

        private readonly string header;
        private readonly string information;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationUnitInformation"/> class.
        /// </summary>
        /// <param name="unit">Configuration unit.</param>
        public ConfigurationUnitInformation(ConfigurationUnit unit)
        {
            this.header = this.CreateHeader(unit, unit.Details != null ? unit.Details.UnitType : unit.Type);
            this.information = this.CreateInformation(unit);
        }

        /// <summary>
        /// Gets the header information message.
        /// </summary>
        /// <returns>Header information message.</returns>
        public HostInformationMessage GetHeader()
        {
            return Utilities.CreateInformationMessage(this.header, foregroundColor: ConsoleColor.Cyan);
        }

        /// <summary>
        /// Gets the information message.
        /// </summary>
        /// <returns>Information message.</returns>
        public HostInformationMessage GetInformation()
        {
            return Utilities.CreateInformationMessage(this.information);
        }

        private string CreateHeader(ConfigurationUnit unit, string name)
        {
            var sb = new StringBuilder();
            sb.Append($"{this.IntentToString(unit.Intent)} :: {name}");

            string identifier = unit.Identifier;
            if (!string.IsNullOrEmpty(identifier))
            {
                sb.Append($" [{identifier}]");
            }

            return sb.ToString();
        }

        private string CreateInformation(ConfigurationUnit unit)
        {
            IConfigurationUnitProcessorDetails details = unit.Details;
            ValueSet metadata = unit.Metadata;

            var sb = new StringBuilder();
            if (details != null)
            {
                this.CreateInformationWithDetails(ref sb, details, metadata);
            }
            else
            {
                this.CreateInformationWithoutDetails(ref sb, metadata);
            }

            // -- Sample output footer --
            //   Dependencies: dep1, dep2, ...
            //   Settings:
            //     <... settings splat>
            var dependencies = unit.Dependencies;
            if (dependencies.Count > 0)
            {
                var dependencySb = new StringBuilder();
                foreach (var dependency in dependencies)
                {
                    dependencySb.Append($" {dependency}");
                }

                sb.AppendLine($"  {string.Format(Resources.ConfigurationDependencies, dependencySb.ToString())}");
            }

            var settings = unit.Settings;
            if (settings.Count > 0)
            {
                sb.AppendLine($"  {Resources.ConfigurationSettings}");
                this.AppendValueSet(ref sb, settings, 4);
            }

            return sb.ToString();
        }

        // -- Sample output when IConfigurationUnitProcessorDetails present --
        // Intent :: UnitName <from details> [Identifier]
        //   UnitDocumentationUri <if present>
        //   Description <from details first, directives second>
        //   "Module": ModuleName "by" Author / Publisher (IsLocal / ModuleSource)
        //     "Signed by": SigningCertificateChain (leaf subject CN)
        //     PublishedModuleUri / ModuleDocumentationUri <if present>
        //     ModuleDescription
        private void CreateInformationWithDetails(ref StringBuilder sb, IConfigurationUnitProcessorDetails details, ValueSet directives)
        {
            var unitDocumentationUri = details.UnitDocumentationUri;
            if (unitDocumentationUri != null)
            {
                sb.AppendLine($"  {unitDocumentationUri.AbsoluteUri}");
            }

            var unitDescriptionFromDetails = details.UnitDescription;
            if (!string.IsNullOrEmpty(unitDescriptionFromDetails))
            {
                sb.AppendLine($"  {unitDescriptionFromDetails}");
            }
            else
            {
                var unitDescriptionFromDirectives = directives.TryGetStringValue(Description);
                if (!string.IsNullOrEmpty(unitDescriptionFromDirectives))
                {
                    sb.AppendLine($"  {unitDescriptionFromDirectives}");
                }
            }

            var author = details.Author;
            if (string.IsNullOrEmpty(author))
            {
                author = details.Publisher;
            }

            if (details.IsLocal)
            {
                sb.AppendLine($"  {string.Format(Resources.ConfigurationModuleWithDetails, details.ModuleName, author, Resources.ConfigurationLocal)}");
            }
            else
            {
                sb.AppendLine($"  {string.Format(Resources.ConfigurationModuleWithDetails, details.ModuleName, author, details.ModuleSource)}");
            }

            // TODO: see signature information in ConfigurationFlow.cpp
            var moduleUri = details.PublishedModuleUri;
            if (moduleUri == null)
            {
                moduleUri = details.ModuleDocumentationUri;
            }

            if (moduleUri != null)
            {
                sb.AppendLine($"    {moduleUri.AbsoluteUri}");
            }

            var moduleDescription = details.ModuleDescription;
            if (!string.IsNullOrEmpty(moduleDescription))
            {
                sb.AppendLine($"    {moduleDescription}");
            }
        }

        // -- Sample output when no IConfigurationUnitProcessorDetails present --
        // Intent :: UnitName <from unit> [identifier]
        //   Description (from directives)
        //   "Module": module <directive>
        private void CreateInformationWithoutDetails(ref StringBuilder sb, ValueSet directives)
        {
            var unitDescriptionFromDirectives = directives.TryGetStringValue(Description);
            if (!string.IsNullOrEmpty(unitDescriptionFromDirectives))
            {
                sb.AppendLine($"  {unitDescriptionFromDirectives}");
            }

            var unitModuleFromDirectives = directives.TryGetStringValue(Module);
            if (!string.IsNullOrEmpty(unitModuleFromDirectives))
            {
                sb.AppendLine($"  {string.Format(Resources.ConfigurationModuleNameOnly, unitModuleFromDirectives)}");
            }
        }

        private void AppendValueSet(ref StringBuilder sb, ValueSet valueSet, int indent)
        {
            var indentString = new string(' ', indent);

            foreach (var value in valueSet)
            {
                sb.Append($"{indentString}{value.Key}:");

                // Can't use IPropertyValue here...
                var obj = value.Value;
                var innerValueSet = obj as ValueSet;
                if (innerValueSet != null)
                {
                    sb.AppendLine();
                    if (innerValueSet.ContainsKey(TreatAsArray))
                    {
                        this.AppendValueSetAsArray(ref sb, innerValueSet, indent + 2);
                    }
                    else
                    {
                        this.AppendValueSet(ref sb, innerValueSet, indent + 2);
                    }
                }
                else
                {
                    this.AppendPropertyValue(ref sb, obj);
                }
            }
        }

        private void AppendValueSetAsArray(ref StringBuilder sb, ValueSet valueSet, int indent)
        {
            var indentString = new string(' ', indent);

            var sortedList = new SortedList<int, object>();

            foreach (var keyValuePair in valueSet)
            {
                if (keyValuePair.Key != TreatAsArray)
                {
                    if (int.TryParse(keyValuePair.Key, out int key))
                    {
                        sortedList.Add(key, keyValuePair.Value);
                    }
                    else
                    {
                        throw new InvalidOperationException(keyValuePair.Key);
                    }
                }
            }

            foreach (var arrayValue in sortedList)
            {
                sb.Append($"{indentString}-");
                var obj = arrayValue.Value;

                var innerValueSet = obj as ValueSet;
                if (innerValueSet == null)
                {
                    this.AppendPropertyValue(ref sb, obj);
                }
                else
                {
                    var size = innerValueSet.Count;
                    if (size > 0)
                    {
                        // First one is special.
                        var first = innerValueSet.First();
                        sb.Append($" {first.Key}:");

                        var firstValueSet = first.Value as ValueSet;
                        if (firstValueSet == null)
                        {
                            this.AppendPropertyValue(ref sb, first.Value);
                        }
                        else
                        {
                            sb.AppendLine();
                            this.AppendValueSet(ref sb, firstValueSet, indent + 4);
                        }

                        if (size > 1)
                        {
                            innerValueSet.Remove(first.Key);
                            this.AppendValueSet(ref sb, innerValueSet, indent + 2);
                            innerValueSet.Add(first);
                        }
                    }
                }
            }
        }

        private void AppendPropertyValue(ref StringBuilder sb, object value)
        {
            Type type = value.GetType();
            if (type == typeof(string))
            {
                sb.AppendLine($" {(string)value}");
            }
            else if (type == typeof(bool))
            {
                string message = (bool)value ? "true" : "false";
                sb.AppendLine($" {message}");
            }
            else if (type == typeof(long))
            {
                sb.AppendLine($" {(long)value}");
            }
            else
            {
                sb.AppendLine($" [Debug:PropertyType={type}]");
            }
        }

        private string IntentToString(ConfigurationUnitIntent intent)
        {
            return intent switch
            {
                ConfigurationUnitIntent.Assert => Resources.ConfigurationAssert,
                ConfigurationUnitIntent.Inform => Resources.ConfigurationInform,
                ConfigurationUnitIntent.Apply => Resources.ConfigurationApply,
                _ => string.Empty,
            };
        }
    }
}
