// -----------------------------------------------------------------------------
// <copyright file="IPowerShellGet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.Helpers
{
    using System.Management.Automation;
    using Microsoft.PowerShell.Commands;
    using SemanticVersion = Microsoft.Management.Configuration.Processor.Helpers.SemanticVersion;

    /// <summary>
    /// Interface for PowerShellGet cmdlets.
    /// </summary>
    internal interface IPowerShellGet
    {
        /// <summary>
        /// Calls Find-Module.
        /// </summary>
        /// <param name="pwsh">PowerShell instance.</param>
        /// <param name="moduleName">Module name.</param>
        /// <param name="semanticVersion">Optional version.</param>
        /// <param name="semanticMinVersion">Optional min version.</param>
        /// <param name="semanticMaxVersion">Optional max version.</param>
        /// <param name="repository">Optional repository.</param>
        /// <param name="allowPrerelease">Optional allow prerelease module.</param>
        /// <returns>Module info, null if not found.</returns>
        PSObject? FindModule(
            PowerShell pwsh,
            string moduleName,
            SemanticVersion? semanticVersion,
            SemanticVersion? semanticMinVersion,
            SemanticVersion? semanticMaxVersion,
            string? repository,
            bool? allowPrerelease);

        /// <summary>
        /// Calls Find-DscResource.
        /// </summary>
        /// <param name="pwsh">PowerShell instance.</param>
        /// <param name="resourceName">resource name.</param>
        /// <param name="moduleName">Optional module name.</param>
        /// <param name="semanticVersion">Optional version.</param>
        /// <param name="semanticMinVersion">Optional min version.</param>
        /// <param name="semanticMaxVersion">Optional max version.</param>
        /// <param name="repository">Optional repository.</param>
        /// <param name="allowPrerelease">Optional allow prerelease module.</param>
        /// <returns>Dsc Resource info, null if not found.</returns>
        PSObject? FindDscResource(
            PowerShell pwsh,
            string resourceName,
            string? moduleName,
            SemanticVersion? semanticVersion,
            SemanticVersion? semanticMinVersion,
            SemanticVersion? semanticMaxVersion,
            string? repository,
            bool? allowPrerelease);

        /// <summary>
        /// Calls Save-Module with module specification.
        /// </summary>
        /// <param name="pwsh">PowerShell instance.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <param name="location">Location to save module.</param>
        void SaveModule(PowerShell pwsh, ModuleSpecification moduleSpecification, string location);

        /// <summary>
        /// Calls Save-Module -InputObject object -Path location.
        /// Input object must be the result of Find cmdlets of PowerShellGet.
        /// </summary>
        /// <param name="pwsh">PowerShell instance.</param>
        /// <param name="inputObject">Input object.</param>
        /// <param name="location">Location to save module.</param>
        void SaveModule(PowerShell pwsh, PSObject inputObject, string location);

        /// <summary>
        /// Calls Install-Module -InputObject object.
        /// Input object must be the result of Find cmdlets of PowerShellGet.
        /// </summary>
        /// <param name="pwsh">PowerShell instance.</param>
        /// <param name="inputObject">Input object.</param>
        /// <param name="allUsers">If to install to all users.</param>
        void InstallModule(PowerShell pwsh, PSObject inputObject, bool allUsers);

        /// <summary>
        /// Calls Install-Module with a module specification.
        /// </summary>
        /// <param name="pwsh">PowerShell instance.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <param name="allUsers">If to install to all users.</param>
        void InstallModule(PowerShell pwsh, ModuleSpecification moduleSpecification, bool allUsers);
    }
}
