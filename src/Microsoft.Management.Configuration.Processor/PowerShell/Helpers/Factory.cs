// -----------------------------------------------------------------------------
// <copyright file="Factory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.Helpers
{
    using Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.Unit;

    /// <summary>
    /// Enables creation of PowerShell specific instances of configuration interfaces.
    /// </summary>
    internal static class Factory
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Factory"/> class.
        /// </summary>
        /// <param name="dscResourceInfo">DSC Resource info.</param>
        /// <returns>An initialized instance of <see cref="ConfigurationUnitSettingDetails"/>.</returns>
        public static ConfigurationUnitSettingDetails CreateUnitSettingDetails(DscResourcePropertyInfoInternal dscResourceInfo)
        {
            return new ConfigurationUnitSettingDetails()
            {
                Identifier = dscResourceInfo.Name,
                IsRequired = dscResourceInfo.IsMandatory,
                Type = GetPropertyType(dscResourceInfo.PropertyType),
            };
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
