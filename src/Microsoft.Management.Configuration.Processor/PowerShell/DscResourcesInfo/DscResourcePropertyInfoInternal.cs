// -----------------------------------------------------------------------------
// <copyright file="DscResourcePropertyInfoInternal.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Contains a DSC resource property information.
    /// </summary>
    internal sealed class DscResourcePropertyInfoInternal
    {
        private const string DscResourcePropertyInfoFullName = "Microsoft.PowerShell.DesiredStateConfiguration.DscResourcePropertyInfo";

        /// <summary>
        /// Initializes a new instance of the <see cref="DscResourcePropertyInfoInternal"/> class.
        /// </summary>
        /// <param name="dscPropertyInfo">Dynamic DSC property info.</param>
        public DscResourcePropertyInfoInternal(dynamic dscPropertyInfo)
        {
            if (dscPropertyInfo.GetType().FullName != DscResourcePropertyInfoFullName)
            {
                throw new ArgumentException();
            }

            this.Name = dscPropertyInfo.Name;
            this.PropertyType = dscPropertyInfo.PropertyType;
            this.IsMandatory = dscPropertyInfo.IsMandatory;
            this.Values = dscPropertyInfo.Values;
        }

        /// <summary>
        /// Gets or sets name of the property.
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets type of the property.
        /// </summary>
        public string PropertyType { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the property is mandatory or not.
        /// </summary>
        public bool IsMandatory { get; set; }

        /// <summary>
        /// Gets Values for a resource property.
        /// </summary>
        public List<string> Values { get; private set; } = new List<string>();
    }
}
