// -----------------------------------------------------------------------------
// <copyright file="ImportModuleException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;

    /// <summary>
    /// Import-Module threw an exception.
    /// </summary>
    internal class ImportModuleException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ImportModuleException"/> class.
        /// </summary>
        /// <param name="moduleName">Module name.</param>
        /// <param name="inner">Inner exception.</param>
        public ImportModuleException(string? moduleName, Exception inner)
            : base($"Could not import module: {moduleName?.ToString() ?? "<no module>"}", inner)
        {
            this.HResult = ErrorCodes.WinGetConfigUnitImportModule;
            this.ModuleName = moduleName;
        }

        /// <summary>
        /// Gets the module name.
        /// </summary>
        public string? ModuleName { get; }
    }
}
