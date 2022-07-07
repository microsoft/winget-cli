// -----------------------------------------------------------------------------
// <copyright file="FilterAttribute.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Attributes
{
    using System;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// Marking a property with this attribute causes it to be added to the filters list when searching for packages.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, AllowMultiple = false, Inherited = true)]
    public class FilterAttribute : Attribute
    {
        /// <summary>
        /// Gets or sets the field that the filter will be matching against.
        /// </summary>
        public PackageMatchField Field { get; set; }
    }
}
