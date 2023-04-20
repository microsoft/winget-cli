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
        private const string MaxRange = "999999999";

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
                // For some reason, the constructor of ModuleSpecification that takes
                // a hashtable calls ModuleCmdletBase.GetMaximumVersion. This method will
                // validate the max version and replace * for 999999999 only if its the last
                // char in the string. But then the returned value is not assigned to the
                // ModuleSpecification's MaximumVersion property. If we want to set a
                // MaximumVersion with a wildcard and pass this to Install-Module it will
                // fail with "Cannot convert value 'x.*' to type 'System.Version'."
                moduleInfo.Add(Parameters.MaximumVersion, maxVersion.Replace("*", MaxRange));
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
