// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitSettingDetails.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Unit
{
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
    using Windows.Foundation.Metadata;

    /// <summary>
    /// Provides information for a specific configuration unit setting.
    /// </summary>
    internal sealed class ConfigurationUnitSettingDetails : IConfigurationUnitSettingDetails
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationUnitSettingDetails"/> class.
        /// </summary>
        /// <param name="dscResourceInfo">DSC Resource info.</param>
        public ConfigurationUnitSettingDetails(DscResourcePropertyInfoInternal dscResourceInfo)
        {
            this.Identifier = dscResourceInfo.Name;
            this.IsRequired = dscResourceInfo.IsMandatory;
            this.Type = GetPropertyType(dscResourceInfo.PropertyType);
        }

        /// <summary>
        /// Gets the name of the setting.
        /// </summary>
        public string Identifier { get; }

        /// <summary>
        /// Gets the title of the setting.
        /// </summary>
        public string Title { get; } = string.Empty;

        /// <summary>
        /// Gets the description of the setting.
        /// </summary>
        public string Description { get; } = string.Empty;

        /// <summary>
        /// Gets a value indicating whether the setting is a key. This is used to determine if different settings are in conflict.
        /// </summary>
        public bool IsKey { get; } = false;

        /// <summary>
        /// Gets a value indicating whether a non-empty value for the setting is required.
        /// </summary>
        public bool IsRequired { get; }

        /// <summary>
        /// Gets a value indicating whether the setting should be serialized in order to be applied on another system.
        /// </summary>
        public bool IsInformational { get; } = false;

        /// <summary>
        /// Gets the data type for the value of this setting.
        /// </summary>
        public Windows.Foundation.PropertyType Type { get; }

        /// <summary>
        /// Gets the semantics to be used for this setting. The goal is to enable richer conflict detection and authoring
        /// scenarios by having a deeper understanding of this value than "String".
        /// </summary>
        public string Schema { get; } = string.Empty;

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
