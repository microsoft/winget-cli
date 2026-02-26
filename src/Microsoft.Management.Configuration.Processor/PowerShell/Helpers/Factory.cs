// -----------------------------------------------------------------------------
// <copyright file="Factory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Management.Automation;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.Unit;
    using Windows.Security.Cryptography.Certificates;

    /// <summary>
    /// Enables creation of PowerShell specific instances of configuration interfaces.
    /// </summary>
    internal static class Factory
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
        /// <param name="certs">List of certificates.</param>
        /// <returns>An initialized instance of <see cref="ConfigurationUnitProcessorDetails"/>.</returns>
        public static ConfigurationUnitProcessorDetails CreateUnitProcessorDetails(
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

            ConfigurationUnitProcessorDetails result = new ConfigurationUnitProcessorDetails
            {
                UnitType = unitName,
            };

            if (dscResourceInfo is not null)
            {
                result.IsLocal = true;

                var settings = new List<IConfigurationUnitSettingDetails>();
                foreach (var properties in dscResourceInfo.Properties)
                {
                    settings.Add(CreateUnitSettingDetails(properties));
                }

                result.Settings = settings;

                if (psModuleInfo is null)
                {
                    result.ModuleName = dscResourceInfo.ModuleName;
                    result.Version = dscResourceInfo.Version is not null ? dscResourceInfo.Version.ToString() : null;
                }
            }

            if (psModuleInfo is not null)
            {
                result.ModuleDocumentationUri = psModuleInfo.HelpInfoUri is not null ? new Uri(psModuleInfo.HelpInfoUri) : null;
                result.UnitIconUri = psModuleInfo.IconUri;
                result.ModuleName = psModuleInfo.Name;
                result.ModuleType = psModuleInfo.ModuleType.ToString();
                result.ModuleDescription = psModuleInfo.Description;
                result.PublishedModuleUri = psModuleInfo.ProjectUri;
                result.Version = psModuleInfo.Version.ToString();
                result.Author = psModuleInfo.Author;
                result.Publisher = psModuleInfo.CompanyName;
            }

            if (getModuleInfo is not null)
            {
                result.ModuleSource = TryGetPropertyAsString(getModuleInfo, "Repository");
                result.PublishedDate = TryGetPropertyFromDateTimeToDateTimeOffset(getModuleInfo, "PublishedDate");

                var repoSourceLocation = getModuleInfo.Properties["RepositorySourceLocation"];
                if (repoSourceLocation is not null)
                {
                    string? repoSourceLocationValue = repoSourceLocation.Value as string;
                    if (repoSourceLocationValue is not null)
                    {
                        result.IsPublic = PublicRepositories.Any(r => r == repoSourceLocationValue);
                    }
                }

                if (psModuleInfo is null)
                {
                    // Type is not the same as this PSModuleType.
                    result.UnitIconUri = TryGetPropertyAsUri(getModuleInfo, "IconUri");
                    result.ModuleName = TryGetPropertyAsString(getModuleInfo, "Name");
                    result.ModuleDescription = TryGetPropertyAsString(getModuleInfo, "Description");
                    result.Version = TryGetPropertyAsString(getModuleInfo, "Version");
                    result.Author = TryGetPropertyAsString(getModuleInfo, "Author");
                    result.Publisher = TryGetPropertyAsString(getModuleInfo, "CompanyName");
                }
            }

            result.SigningInformation = certs;

            return result;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Factory"/> class.
        /// </summary>
        /// <param name="dscResourceInfo">DSC Resource info.</param>
        /// <returns>An initialized instance of <see cref="ConfigurationUnitSettingDetails"/>.</returns>
        public static ConfigurationUnitSettingDetails CreateUnitSettingDetails(DscResourcePropertyInfoInternal dscResourceInfo)
        {
            return new ConfigurationUnitSettingDetails
            {
                Identifier = dscResourceInfo.Name,
                IsRequired = dscResourceInfo.IsMandatory,
                Type = GetPropertyType(dscResourceInfo.PropertyType),
            };
        }

        private static string? TryGetPropertyAsString(PSObject getModuleInfo, string getModuleInfoProperty)
        {
            var moduleProperty = getModuleInfo.Properties[getModuleInfoProperty];
            if (moduleProperty is not null)
            {
                return moduleProperty.Value as string;
            }

            return null;
        }

        private static Uri? TryGetPropertyAsUri(PSObject getModuleInfo, string getModuleInfoProperty)
        {
            var moduleProperty = getModuleInfo.Properties[getModuleInfoProperty];
            if (moduleProperty is not null)
            {
                string? modulePropertyString = moduleProperty.Value as string;
                if (modulePropertyString is not null)
                {
                    return new Uri(modulePropertyString);
                }
            }

            return null;
        }

        private static DateTimeOffset TryGetPropertyFromDateTimeToDateTimeOffset(PSObject getModuleInfo, string getModuleInfoProperty)
        {
            var moduleProperty = getModuleInfo.Properties[getModuleInfoProperty];
            if (moduleProperty is not null)
            {
                DateTime propertyAsDateTime;

                try
                {
                    propertyAsDateTime = (DateTime)moduleProperty.Value;
                    return new DateTimeOffset(propertyAsDateTime);
                }
                catch
                {
                }
            }

            return default;
        }

        private static Windows.Foundation.PropertyType GetPropertyType(string propertyType)
        {
            switch (propertyType.ToLowerInvariant())
            {
                case "[byte]": return Windows.Foundation.PropertyType.UInt8;
                case "[int16]": return Windows.Foundation.PropertyType.Int16;
                case "[uint16]": return Windows.Foundation.PropertyType.UInt16;
                case "[int32]": return Windows.Foundation.PropertyType.Int32;
                case "[uint32]": return Windows.Foundation.PropertyType.UInt32;
                case "[int64]": return Windows.Foundation.PropertyType.Int64;
                case "[uint64]": return Windows.Foundation.PropertyType.UInt64;
                case "[single]": return Windows.Foundation.PropertyType.Single;
                case "[double]": return Windows.Foundation.PropertyType.Double;
                case "[char]": return Windows.Foundation.PropertyType.Char16;
                case "[bool]": return Windows.Foundation.PropertyType.Boolean;
                case "[string]": return Windows.Foundation.PropertyType.String;
                case "[datetime]": return Windows.Foundation.PropertyType.DateTime;
                case "[datetimeoffset]": return Windows.Foundation.PropertyType.DateTime;
                case "[timespan]": return Windows.Foundation.PropertyType.TimeSpan;
                case "[guid]": return Windows.Foundation.PropertyType.Guid;
                case "[byte[]]": return Windows.Foundation.PropertyType.UInt8Array;
                case "[int16[]]": return Windows.Foundation.PropertyType.Int16Array;
                case "[uint16[]]": return Windows.Foundation.PropertyType.UInt16Array;
                case "[int32[]]": return Windows.Foundation.PropertyType.Int32Array;
                case "[uint32[]]": return Windows.Foundation.PropertyType.UInt32Array;
                case "[int64[]]": return Windows.Foundation.PropertyType.Int64Array;
                case "[uint64[]]": return Windows.Foundation.PropertyType.UInt64Array;
                case "[single[]]": return Windows.Foundation.PropertyType.SingleArray;
                case "[double[]]": return Windows.Foundation.PropertyType.DoubleArray;
                case "[char[]]": return Windows.Foundation.PropertyType.Char16Array;
                case "[bool[]]": return Windows.Foundation.PropertyType.BooleanArray;
                case "[string[]]": return Windows.Foundation.PropertyType.StringArray;
                case "[object[]]": return Windows.Foundation.PropertyType.InspectableArray;
                case "[datetime[]]": return Windows.Foundation.PropertyType.DateTimeArray;
                case "[datetimeoffset[]]": return Windows.Foundation.PropertyType.DateTimeArray;
                case "[timespan[]]": return Windows.Foundation.PropertyType.TimeSpanArray;
                case "[guid[]]": return Windows.Foundation.PropertyType.GuidArray;

                // Everything else will just be an object...
                default: return Windows.Foundation.PropertyType.Inspectable;
            }
        }
    }
}
