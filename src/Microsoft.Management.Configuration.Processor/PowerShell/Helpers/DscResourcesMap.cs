// -----------------------------------------------------------------------------
// <copyright file="DscResourcesMap.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo;

    /// <summary>
    /// DscResources in the system. Get-DscResource looks the entire PSModulePath to find resources every time, so
    /// calling it multiples times is expensive. This class will store them in memory per configuration set.
    /// The resources are stored in a dictionary in the following hierarchy.
    /// - Module name
    ///   - Resource name
    ///     - Module Version
    ///       - DscResourceInfo obj.
    /// There are some resources that don't have a module name. These are modules that live under
    /// C:\WINDOWS\system32\WindowsPowershell\v1.0\Modules\PsDesiredStateConfiguration\DscResources.
    /// This is not currently used. For now, we let PowerShell handle all the versioning. In order to avoid
    /// calls to Get-DscResource every time a unit is set, this class needs to be updated to handle min and max
    /// version. As it is, it just does RequiredVersion.
    /// </summary>
    internal sealed class DscResourcesMap
    {
        private readonly Dictionary<string, ModuleDscResources> resources = new Dictionary<string, ModuleDscResources>();

        /// <summary>
        /// Initializes a new instance of the <see cref="DscResourcesMap"/> class.
        /// </summary>
        /// <param name="resources">DSC Resources.</param>
        public DscResourcesMap(IReadOnlyList<DscResourceInfoInternal> resources)
        {
            foreach (var resource in resources)
            {
                if (string.IsNullOrEmpty(resource.Name))
                {
                    throw new ArgumentException(nameof(resource.Name));
                }

                if (resource.Version is null)
                {
                    throw new ArgumentException(nameof(resource.Version));
                }

                this.Insert(resource);
            }
        }

        /// <summary>
        /// Looks for a DSC resource in the specified module.
        /// </summary>
        /// <param name="dscResourceName">DSC resource name.</param>
        /// <param name="moduleName">Optional module name.</param>
        /// <param name="version">Optional module version.</param>
        /// <returns>DscResourceInfoInternal. Null if not found.</returns>
        public DscResourceInfoInternal? GetResource(string dscResourceName, string? moduleName, Version? version)
        {
            string normalizedResourceName = StringHelpers.Normalize(dscResourceName);
            string? normalizedModuleName = null;
            if (moduleName is not null)
            {
                normalizedModuleName = StringHelpers.Normalize(moduleName);
            }

            if (normalizedModuleName is null)
            {
                foreach (var module in this.resources.Values)
                {
                    var resourceInfo = module.GetDscResourceInfo(normalizedResourceName, version);
                    if (resourceInfo is not null)
                    {
                        return resourceInfo;
                    }
                }
            }
            else
            {
                if (this.resources.TryGetValue(normalizedModuleName, out ModuleDscResources? module))
                {
                    return module.GetDscResourceInfo(normalizedResourceName, version);
                }
            }

            return null;
        }

        /// <summary>
        /// Does a DSC resource exists.
        /// </summary>
        /// <param name="dscResourceName">DSC resource name.</param>
        /// <param name="moduleName">Optional module name.</param>
        /// <param name="version">Optional module version.</param>
        /// <returns>True if resource exists.</returns>
        public bool Exists(string dscResourceName, string? moduleName, Version? version)
        {
            return this.GetResource(dscResourceName, moduleName, version) != null;
        }

        private void Insert(DscResourceInfoInternal dscResourceInfo)
        {
            string? normalizedModuleName = dscResourceInfo.NormalizedModuleName;

            // There are some resources that doesn't have module, use empty as key.
            string moduleKey = string.Empty;
            if (normalizedModuleName is not null)
            {
                moduleKey = normalizedModuleName;
            }

            if (!this.resources.ContainsKey(moduleKey))
            {
                this.resources.Add(moduleKey, new ModuleDscResources(moduleKey, dscResourceInfo));
            }
            else
            {
                this.resources[moduleKey].AddResource(moduleKey, dscResourceInfo);
            }
        }

        /// <summary>
        /// Represents a map versions for a DscResource.
        /// </summary>
        private class DscResourceVersions : Dictionary<Version, DscResourceInfoInternal>
        {
            private readonly string resourceName;

            public DscResourceVersions(DscResourceInfoInternal dscResourceInfo)
            {
                this.resourceName = dscResourceInfo.NormalizedName;
                this.AddVersion(dscResourceInfo);
            }

            /// <summary>
            /// Adds a resource version.
            /// </summary>
            /// <param name="dscResourceInfo">DscResourceInfo.</param>
            public void AddVersion(DscResourceInfoInternal dscResourceInfo)
            {
                if (this.resourceName != dscResourceInfo.NormalizedName)
                {
                    throw new ArgumentException(dscResourceInfo.NormalizedName);
                }

                // There are some system resources without version or module name.
                if (dscResourceInfo.Version is null)
                {
                    this.Add(new Version(), dscResourceInfo);
                }
                else
                {
                    // Get-DscResource will fail if the same module with same
                    // resources and same version exists in the module path
                    // so this shouldn't happen. Either way, we should throw there or here.
                    this.Add(dscResourceInfo.Version, dscResourceInfo);
                }
            }

            public DscResourceInfoInternal? GetDscResourceInfo(Version? version)
            {
                if (version is null)
                {
                    // Sort keys, get the latest.
                    var newestVersion = this.Keys.OrderByDescending(v => v).First();
                    return this[newestVersion];
                }

                if (this.TryGetValue(version, out DscResourceInfoInternal? resourceInfo))
                {
                    return resourceInfo;
                }

                return null;
            }
        }

        /// <summary>
        /// Represent a map of resources for a module.
        /// </summary>
        private class ModuleDscResources : Dictionary<string, DscResourceVersions>
        {
            private readonly string normalizedModuleName;

            public ModuleDscResources(string normalizedModuleName, DscResourceInfoInternal dscResourceInfo)
            {
                this.normalizedModuleName = normalizedModuleName;
                this.AddResource(this.normalizedModuleName, dscResourceInfo);
            }

            public void AddResource(string normalizedModuleName, DscResourceInfoInternal dscResourceInfo)
            {
                if (this.normalizedModuleName != normalizedModuleName)
                {
                    throw new ArgumentException(nameof(normalizedModuleName));
                }

                var normalizedResourceName = dscResourceInfo.NormalizedName;
                if (!this.ContainsKey(normalizedResourceName))
                {
                    this.Add(normalizedResourceName, new DscResourceVersions(dscResourceInfo));
                }
                else
                {
                    this[normalizedResourceName].AddVersion(dscResourceInfo);
                }
            }

            public DscResourceInfoInternal? GetDscResourceInfo(string dscResourceName, Version? version)
            {
                if (this.TryGetValue(dscResourceName, out DscResourceVersions? versions))
                {
                    return versions.GetDscResourceInfo(version);
                }

                return null;
            }
        }
    }
}
