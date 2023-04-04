// -----------------------------------------------------------------------------
// <copyright file="InvokeDscResourceSetException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// A call to Invoke-DscResource Set failed unexpectedly.
    /// </summary>
    internal class InvokeDscResourceSetException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InvokeDscResourceSetException"/> class.
        /// </summary>
        /// <param name="resourceName">Resource name.</param>
        /// <param name="module">Optional module.</param>
        public InvokeDscResourceSetException(string resourceName, ModuleSpecification? module)
            : base(CreateMessage(resourceName, module, null))
        {
            this.HResult = ErrorCodes.WinGetConfigUnitInvokeSet;
            this.ResourceName = resourceName;
            this.Module = module;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="InvokeDscResourceSetException"/> class.
        /// </summary>
        /// <param name="resourceName">Resource name.</param>
        /// <param name="module">Optional module.</param>
        /// <param name="message">Message.</param>
        public InvokeDscResourceSetException(string resourceName, ModuleSpecification? module, string message)
            : base(CreateMessage(resourceName, module, message))
        {
            this.HResult = ErrorCodes.WinGetConfigUnitInvokeSet;
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

        private static string CreateMessage(string resourceName, ModuleSpecification? module, string? message)
        {
            string result = $"Failed when calling `Set` for resource: {resourceName} [{module?.ToString() ?? "<no module>"}]";
            if (message != null)
            {
                result += $" Message: '{message}'";
            }

            return result;
        }
    }
}
