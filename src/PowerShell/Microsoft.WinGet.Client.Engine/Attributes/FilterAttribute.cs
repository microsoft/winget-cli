// -----------------------------------------------------------------------------
// <copyright file="FilterAttribute.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Attributes
{
    using System;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// A <see cref="FindPackagesOptions" /> is constructed by introspecting on the inheritance tree and
    /// looking for parameters that are marked with this attribute. Properties that are marked with this
    /// attribute are added to the <see cref="FindPackagesOptions" /> object.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, AllowMultiple = false, Inherited = true)]
    internal class FilterAttribute : Attribute
    {
        /// <summary>
        /// Gets or sets the field that the filter will be matching against.
        /// </summary>
        public PackageMatchField Field { get; set; }
    }
}
