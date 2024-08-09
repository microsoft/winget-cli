// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitProcessorDetails.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Unit
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Management.Automation;
    using System.Reflection;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
    using Windows.Security.Cryptography.Certificates;

    /// <summary>
    /// Provides information for a specific configuration unit within the runtime.
    /// </summary>
    internal sealed class ConfigurationUnitProcessorDetails : IConfigurationUnitProcessorDetails
    {
        private static readonly IEnumerable<string> PublicRepositories = new string[]
        {
            "https://www.powershellgallery.com/api/v2",
        };

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationUnitProcessorDetails"/> class.
        /// </summary>
        /// <param name="unitName">Unit name.</param>
        /// <param name="dscResourceInfo">DSC Resource Info.</param>
        /// <param name="psModuleInfo">PSModuleInfo.</param>
        /// <param name="getModuleInfo">GetModuleInfo.</param>
        /// <param name="certs">List of certificates..</param>
        public ConfigurationUnitProcessorDetails(
            string unitName,
            DscResourceInfoInternal? dscResourceInfo,
            PSModuleInfo? psModuleInfo,
            PSObject? getModuleInfo,
            List<Certificate>? certs)
        {
            if (dscResourceInfo is null &&
                psModuleInfo is null &&
                getModuleInfo is null)
            {
                throw new ArgumentException();
            }

            this.UnitType = unitName;

            if (dscResourceInfo is not null)
            {
                this.IsLocal = true;

                var settings = new List<IConfigurationUnitSettingDetails>();
                foreach (var properties in dscResourceInfo.Properties)
                {
                    settings.Add(new ConfigurationUnitSettingDetails(properties));
                }

                this.Settings = settings;

                if (psModuleInfo is null)
                {
                    this.ModuleName = dscResourceInfo.ModuleName;
                    this.Version = dscResourceInfo.Version is not null ? dscResourceInfo.Version.ToString() : null;
                }
            }

            if (psModuleInfo is not null)
            {
                this.ModuleDocumentationUri = psModuleInfo.HelpInfoUri is not null ? new Uri(psModuleInfo.HelpInfoUri) : null;
                this.UnitIconUri = psModuleInfo.IconUri;
                this.ModuleName = psModuleInfo.Name;
                this.ModuleType = psModuleInfo.ModuleType.ToString();
                this.ModuleDescription = psModuleInfo.Description;
                this.PublishedModuleUri = psModuleInfo.ProjectUri;
                this.Version = psModuleInfo.Version.ToString();
                this.Author = psModuleInfo.Author;
                this.Publisher = psModuleInfo.CompanyName;
            }

            if (getModuleInfo is not null)
            {
                this.TrySetPropertyAsString(getModuleInfo, "Repository", nameof(this.ModuleSource));
                this.TrySetPropertyFromDateTimeToDateTimeOffset(getModuleInfo, "PublishedDate", nameof(this.PublishedDate));

                var repoSourceLocation = getModuleInfo.Properties["RepositorySourceLocation"];
                if (repoSourceLocation is not null)
                {
                    string? repoSourceLocationValue = repoSourceLocation.Value as string;
                    if (repoSourceLocationValue is not null)
                    {
                        this.IsPublic = PublicRepositories.Any(r => r == repoSourceLocationValue);
                    }
                }

                if (psModuleInfo is null)
                {
                    // Type is not the same as this PSModuleType.
                    this.TrySetPropertyAsUri(getModuleInfo, "IconUri", nameof(this.UnitIconUri));
                    this.TrySetPropertyAsString(getModuleInfo, "Name", nameof(this.ModuleName));
                    this.TrySetPropertyAsString(getModuleInfo, "Description", nameof(this.ModuleDescription));
                    this.TrySetPropertyAsString(getModuleInfo, "Version", nameof(this.Version));
                    this.TrySetPropertyAsString(getModuleInfo, "Author", nameof(this.Author));
                    this.TrySetPropertyAsString(getModuleInfo, "CompanyName", nameof(this.Publisher));
                }
            }

            this.SigningInformation = certs;
        }

        /// <summary>
        /// Gets the name of the unit of configuration.
        /// </summary>
        public string UnitType { get; private set; }

        /// <summary>
        /// Gets the description of the unit of configuration.
        /// </summary>
        public string? UnitDescription { get; private set; }

        /// <summary>
        /// Gets the URI of the documentation for the unit of configuration.
        /// </summary>
        public Uri? UnitDocumentationUri { get; private set; }

        /// <summary>
        /// Gets the URI of the icon for the unit of configuration.
        /// </summary>
        public Uri? UnitIconUri { get; private set; }

        /// <summary>
        /// Gets the name of the module containing the unit of configuration.
        /// </summary>
        public string? ModuleName { get; private set; }

        /// <summary>
        /// Gets the type of the module containing the unit of configuration.
        /// </summary>
        public string? ModuleType { get; private set; }

        /// <summary>
        /// Gets the source of the module containing the unit of configuration.
        /// </summary>
        public string? ModuleSource { get; private set; }

        /// <summary>
        /// Gets the description of the module containing the unit of configuration.
        /// </summary>
        public string? ModuleDescription { get; private set; }

        /// <summary>
        /// Gets the URI of the documentation for the module containing the unit of configuration.
        /// </summary>
        public Uri? ModuleDocumentationUri { get; private set; }

        /// <summary>
        /// Gets the URI for the published module containing the unit of configuration.
        /// </summary>
        public Uri? PublishedModuleUri { get; private set; }

        /// <summary>
        /// Gets the version of the module containing the unit of configuration.
        /// </summary>
        public string? Version { get; private set; }

        /// <summary>
        /// Gets the publishing date of the module containing the unit of configuration.
        /// </summary>
        public DateTimeOffset PublishedDate { get; private set; }

        /// <summary>
        /// Gets a value indicating whether the module is already present on the system.
        /// </summary>
        public bool IsLocal { get; private set; }

        /// <summary>
        /// Gets the author of the module containing the unit of configuration.
        /// </summary>
        public string? Author { get; private set; }

        /// <summary>
        /// Gets the publisher of the module containing the unit of configuration.
        /// </summary>
        public string? Publisher { get; private set; }

        /// <summary>
        /// Gets the signing certificate of the module files containing the unit of configuration.
        /// </summary>
        public IReadOnlyList<object>? SigningInformation { get; private set; }

        /// <summary>
        /// Gets the settings information for the unit of configuration.
        /// </summary>
        public IReadOnlyList<IConfigurationUnitSettingDetails>? Settings { get; }

        /// <summary>
        /// Gets a value indicating whether the module comes from a public repository.
        /// </summary>
        public bool IsPublic { get; private set; }

        private void TrySetPropertyAsString(PSObject getModuleInfo, string getModuleInfoProperty, string propertyName)
        {
            Type meType = typeof(ConfigurationUnitProcessorDetails);
            PropertyInfo? propertyInfo = meType.GetProperty(propertyName);
            if (propertyInfo is not null)
            {
                var moduleProperty = getModuleInfo.Properties[getModuleInfoProperty];
                if (moduleProperty is not null)
                {
                    propertyInfo.SetValue(this, moduleProperty.Value as string);
                }
                else
                {
                    propertyInfo.SetValue(this, null);
                }
            }
        }

        private void TrySetPropertyAsUri(PSObject getModuleInfo, string getModuleInfoProperty, string propertyName)
        {
            Type meType = typeof(ConfigurationUnitProcessorDetails);
            PropertyInfo? propertyInfo = meType.GetProperty(propertyName);
            if (propertyInfo is not null)
            {
                var moduleProperty = getModuleInfo.Properties[getModuleInfoProperty];
                if (moduleProperty is not null)
                {
                    string? modulePropertyString = moduleProperty.Value as string;
                    if (modulePropertyString is not null)
                    {
                        propertyInfo.SetValue(this, new Uri(modulePropertyString));
                    }
                }
            }
        }

        private void TrySetPropertyFromDateTimeToDateTimeOffset(PSObject getModuleInfo, string getModuleInfoProperty, string propertyName)
        {
            Type meType = typeof(ConfigurationUnitProcessorDetails);
            PropertyInfo? propertyInfo = meType.GetProperty(propertyName);
            if (propertyInfo is not null)
            {
                var moduleProperty = getModuleInfo.Properties[getModuleInfoProperty];
                if (moduleProperty is not null)
                {
                    DateTime propertyAsDateTime;

                    try
                    {
                        propertyAsDateTime = (DateTime)moduleProperty.Value;
                    }
                    catch
                    {
                        return;
                    }

                    propertyInfo.SetValue(this, new DateTimeOffset(propertyAsDateTime));
                }
            }
        }
    }
}
