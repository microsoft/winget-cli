// -----------------------------------------------------------------------------
// <copyright file="ImportModuleException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;
    using System.Management.Automation;

    /// <summary>
    /// Import-Module threw an exception.
    /// </summary>
    internal class ImportModuleException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ImportModuleException"/> class.
        /// </summary>
        /// <param name="moduleName">Module name.</param>
        /// <param name="pwshEx">Inner exception.</param>
        public ImportModuleException(string? moduleName, Exception pwshEx)
            : base($"Could not import module: {moduleName?.ToString() ?? "<no module>"}", pwshEx)
        {
            this.HResult = this.GetHResult(pwshEx);
            this.ModuleName = moduleName;
        }

        /// <summary>
        /// Gets the module name.
        /// </summary>
        public string? ModuleName { get; }

        private int GetHResult(Exception pwshEx)
        {
            if (pwshEx.InnerException is not null)
            {
                var scriptEx = pwshEx.InnerException as ScriptRequiresException;
                if (scriptEx is not null)
                {
                    if (scriptEx.ErrorRecord.CategoryInfo.Category == ErrorCategory.PermissionDenied)
                    {
                        return ErrorCodes.WinGetConfigUnitImportModuleAdmin;
                    }
                }
            }

            return ErrorCodes.WinGetConfigUnitImportModule;
        }
    }
}
