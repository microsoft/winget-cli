// -----------------------------------------------------------------------------
// <copyright file="DscResourceInfoInternal.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation;
    using Microsoft.Management.Configuration.Processor.Helpers;

    /// <summary>
    /// Contains a DSC resource information.
    /// This object should be initialized with a Microsoft.PowerShell.DesiredStateConfiguration.DscResourceInfo.
    /// That object is the result of Get-DscResource and is not the same as System.Management.Automation.DscResourceInfo.
    /// The type is defined in a psd1 in the PSDesiredStateConfiguration. This file is based on that.
    /// If Invoke-DscResource gets support for passing the DscResourceInfo, we could keep the original object as member
    /// to then pass it in code.
    /// </summary>
    internal class DscResourceInfoInternal
    {
        private const string DscResourceInfoFullName = "Microsoft.PowerShell.DesiredStateConfiguration.DscResourceInfo";

        /// <summary>
        /// Initializes a new instance of the <see cref="DscResourceInfoInternal"/> class.
        /// </summary>
        /// <param name="info">Dynamic object. Expected DscResourceInfo.</param>
        public DscResourceInfoInternal(dynamic info)
        {
            if (info.GetType().FullName != DscResourceInfoFullName)
            {
                throw new ArgumentException();
            }

            this.ResourceType = info.ResourceType;
            this.Name = info.Name;
            this.FriendlyName = info.FriendlyName;
            this.Module = info.Module;
            this.Path = info.Path;
            this.ParentPath = info.ParentPath;
            this.ImplementedAs = Enum.Parse<ImplementedAsTypeInternal>(info.ImplementedAs.ToString());
            this.CompanyName = info.CompanyName;

            if (this.Module is not null)
            {
                this.ModuleName = this.Module.Name;
                this.Version = this.Module.Version;
            }

            foreach (object property in info.Properties)
            {
                this.Properties.Add(new DscResourcePropertyInfoInternal(property));
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DscResourceInfoInternal"/> class.
        /// Used for unit tests.
        /// </summary>
        /// <param name="name">Resource name.</param>
        /// <param name="moduleName">Module name.</param>
        /// <param name="version">Version.</param>
        internal DscResourceInfoInternal(string name, string? moduleName, Version? version)
        {
            this.Name = name;
            this.ModuleName = moduleName;
            this.Version = version;
        }

        /// <summary>
        /// Gets or sets resource type name.
        /// </summary>
        public string? ResourceType { get; set; }

        /// <summary>
        /// Gets or sets Name of the resource. This name is used to access the resource.
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Gets the normalized resource name.
        /// </summary>
        public string NormalizedName
        {
            get { return StringHelpers.Normalize(this.Name); }
        }

        /// <summary>
        /// Gets or sets friendly name defined for the resource.
        /// </summary>
        public string? FriendlyName { get; set; }

        /// <summary>
        /// Gets or sets module which implements the resource. This could point to parent module, if the DSC resource is implemented.
        /// by one of nested modules.
        /// </summary>
        public PSModuleInfo? Module { get; set; }

        /// <summary>
        /// Gets or sets name of the module which implements the resource.
        /// </summary>
        public string? ModuleName { get; set; }

        /// <summary>
        /// Gets the normalized module name.
        /// </summary>
        public string? NormalizedModuleName
        {
            get { return this.ModuleName is not null ? StringHelpers.Normalize(this.ModuleName) : null; }
        }

        /// <summary>
        /// Gets or sets version of the module which implements the resource.
        /// </summary>
        public Version? Version { get; set; }

        /// <summary>
        /// Gets or sets of the file which implements the resource. For the resources which are defined using
        /// MOF file, this will be path to a module which resides in the same folder where schema.mof file is present.
        /// For composite resources, this will be the module which implements the resource.
        /// </summary>
        public string? Path { get; set; }

        /// <summary>
        /// Gets or sets parent folder, where the resource is defined
        /// It is the folder containing either the implementing module(=Path) or folder containing ".schema.mof".
        /// For native providers, Path will be null and only ParentPath will be present.
        /// </summary>
        public string? ParentPath { get; set; }

        /// <summary>
        /// Gets or sets a value which indicate how DSC resource is implemented.
        /// </summary>
        public ImplementedAsTypeInternal? ImplementedAs { get; set; }

        /// <summary>
        /// Gets or sets company which owns this resource.
        /// </summary>
        public string? CompanyName { get; set; }

        /// <summary>
        /// Gets properties of the resource.
        /// </summary>
        public List<DscResourcePropertyInfoInternal> Properties { get; private set; } = new List<DscResourcePropertyInfoInternal>();

        /// <summary>
        /// Updates properties of the resource.
        /// </summary>
        /// <param name="properties">Updated properties.</param>
        public void UpdateProperties(List<DscResourcePropertyInfoInternal> properties)
        {
            this.Properties = properties;
        }
    }
}
