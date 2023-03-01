// -----------------------------------------------------------------------------
// <copyright file="PowerShellHelpers.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Helpers
{
    using System.Collections;
    using Microsoft.PowerShell.Commands;
    using static Microsoft.Management.Configuration.Processor.Constants.PowerShellConstants;

    /// <summary>
    /// General PowerShell helpers.
    /// </summary>
    internal static class PowerShellHelpers
    {
        /// <summary>
        /// Creates a module specification object.
        /// </summary>
        /// <param name="moduleName">Module name.</param>
        /// <param name="version">Optional version.</param>
        /// <param name="minVersion">Optional min version.</param>
        /// <param name="maxVersion">Optional max version.</param>
        /// <param name="guid">Optional guid.</param>
        /// <returns>ModuleSpecification.</returns>
        public static ModuleSpecification CreateModuleSpecification(
            string moduleName,
            string? version = null,
            string? minVersion = null,
            string? maxVersion = null,
            string? guid = null)
        {
            if (version is null &&
                minVersion is null &&
                maxVersion is null)
            {
                // Otherwise will fail with MissingMemberException...
                return new ModuleSpecification(moduleName);
            }

            var moduleInfo = new Hashtable
                {
                    { Parameters.ModuleName, moduleName },
                };

            if (!string.IsNullOrEmpty(version))
            {
                moduleInfo.Add(Parameters.RequiredVersion, version);
            }

            if (!string.IsNullOrEmpty(minVersion))
            {
                moduleInfo.Add(Parameters.ModuleVersion, minVersion);
            }

            if (!string.IsNullOrEmpty(maxVersion))
            {
                moduleInfo.Add(Parameters.MaximumVersion, maxVersion);
            }

            if (!string.IsNullOrEmpty(guid))
            {
                moduleInfo.Add(Parameters.Guid, guid);
            }

            // Using the Hashtable constructor will verify that RequiredVersion is used properly.
            return new ModuleSpecification(moduleInfo);
        }
    }
}
