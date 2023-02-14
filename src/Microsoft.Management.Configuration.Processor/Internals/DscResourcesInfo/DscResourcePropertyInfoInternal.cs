// -----------------------------------------------------------------------------
// <copyright file="DscResourcePropertyInfoInternal.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Internals.DscResourcesInfo
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.Processor.Internals.Helpers;

    /// <summary>
    /// Contains a DSC resource property information.
    /// </summary>
    internal sealed class DscResourcePropertyInfoInternal
    {
        private const string DscResourcePropertyInfoFullName = "Microsoft.PowerShell.DesiredStateConfiguration.DscResourcePropertyInfo";

        /// <summary>
        /// Initializes a new instance of the <see cref="DscResourcePropertyInfoInternal"/> class.
        /// </summary>
        /// <param name="propertyInfo">Dynamic property info.</param>
        public DscResourcePropertyInfoInternal(dynamic propertyInfo)
        {
            if (propertyInfo.GetType().FullName != DscResourcePropertyInfoFullName)
            {
                throw new ArgumentException();
            }

            if (TypeHelpers.PropertyWithTypeExists<string>(propertyInfo, nameof(this.Name)))
            {
                this.Name = propertyInfo.Name;
            }

            if (TypeHelpers.PropertyWithTypeExists<string>(propertyInfo, nameof(this.PropertyType)))
            {
                this.PropertyType = propertyInfo.PropertyType;
            }

            if (TypeHelpers.PropertyWithTypeExists<bool>(propertyInfo, nameof(this.IsMandatory)))
            {
                this.IsMandatory = propertyInfo.IsMandatory;
            }

            this.Values = new List<string>();
            if (TypeHelpers.PropertyWithTypeExists<List<string>>(propertyInfo, nameof(this.Values)))
            {
                this.Values = propertyInfo.Values;
            }
        }

        /// <summary>
        /// Gets or sets name of the property.
        /// </summary>
        public string? Name { get; set; }

        /// <summary>
        /// Gets or sets type of the property.
        /// </summary>
        public string? PropertyType { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the property is mandatory or not.
        /// </summary>
        public bool? IsMandatory { get; set; }

        /// <summary>
        /// Gets Values for a resource property.
        /// </summary>
        public List<string> Values { get; private set; }
    }
}
