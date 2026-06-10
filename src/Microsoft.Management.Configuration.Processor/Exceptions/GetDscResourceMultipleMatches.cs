// -----------------------------------------------------------------------------
// <copyright file="GetDscResourceMultipleMatches.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// A call to Get-DscResource return multiple results for a specific resource.
    /// </summary>
    internal class GetDscResourceMultipleMatches : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GetDscResourceMultipleMatches"/> class.
        /// </summary>
        /// <param name="resourceName">Resource name.</param>
        /// <param name="module">Optional module.</param>
        public GetDscResourceMultipleMatches(string resourceName, ModuleSpecification? module)
            : base($"Multiple matches found for resource: {resourceName} [{module?.ToString() ?? "<no module>"}]")
        {
            this.HResult = ErrorCodes.WinGetConfigUnitMultipleMatches;
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
