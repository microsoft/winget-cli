// -----------------------------------------------------------------------------
// <copyright file="InstallDscResourceException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// Installing a DSC resource failed unexpectedly.
    /// </summary>
    internal class InstallDscResourceException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InstallDscResourceException"/> class.
        /// </summary>
        /// <param name="resourceName">Resource name.</param>
        /// <param name="module">Module.</param>
        public InstallDscResourceException(string resourceName, ModuleSpecification? module)
            : base($"Unable to find resource after install: {resourceName} [{module?.ToString() ?? "<no module>"}]")
        {
            this.HResult = ErrorCodes.WinGetConfigUnitNotFound;
            this.ResourceName = resourceName;
            this.Module = module;
        }

        /// <summary>
        /// Gets the resource name.
        /// </summary>
        public string ResourceName { get; }

        /// <summary>
        /// Gets the module, if any.
        /// </summary>
        public ModuleSpecification? Module { get; }
    }
}
