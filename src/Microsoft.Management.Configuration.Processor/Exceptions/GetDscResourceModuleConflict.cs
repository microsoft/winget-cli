// -----------------------------------------------------------------------------
// <copyright file="GetDscResourceModuleConflict.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// A call to Get-DscResource failed because at least two modules with the same version where found in the module path.
    /// If you are getting this verify the module path.
    /// </summary>
    internal class GetDscResourceModuleConflict : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GetDscResourceModuleConflict"/> class.
        /// </summary>
        /// <param name="resourceName">Resource name.</param>
        /// <param name="module">Optional module.</param>
        /// <param name="inner">The original runtime exception thrown.</param>
        public GetDscResourceModuleConflict(string? resourceName, ModuleSpecification? module, RuntimeException inner)
            : base($"Multiple modules with same version in module path: {resourceName?.ToString() ?? "<no resource>"} [{module?.ToString() ?? "<no module>"}]", inner)
        {
            this.HResult = ErrorCodes.WinGetConfigUnitModuleConflict;
            this.ResourceName = resourceName;
            this.Module = module;
        }

        /// <summary>
        /// Gets the resource name.
        /// </summary>
        public string? ResourceName { get; }

        /// <summary>
        /// Gets the module, if any.
        /// </summary>
        public ModuleSpecification? Module { get; }
    }
}
