// -----------------------------------------------------------------------------
// <copyright file="FindDscResourceNotFoundException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// Resource not found by Find-DscResource.
    /// </summary>
    internal class FindDscResourceNotFoundException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FindDscResourceNotFoundException"/> class.
        /// </summary>
        /// <param name="resourceName">Resource name.</param>
        /// <param name="module">Optional module.</param>
        public FindDscResourceNotFoundException(string resourceName, ModuleSpecification? module)
            : base($"Could not find resource: {resourceName} [{module?.ToString() ?? "<no module>"}]")
        {
            this.HResult = ErrorCodes.WinGetConfigUnitNotFoundRepository;
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
