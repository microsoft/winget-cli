// -----------------------------------------------------------------------------
// <copyright file="InvokeDscResourceException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.Management.Configuration;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// A call to Invoke-DscResource failed unexpectedly.
    /// </summary>
    internal class InvokeDscResourceException : Exception, IConfigurationUnitResultException
    {
        /// <summary>
        /// The string for the Get method.
        /// </summary>
        public const string Get = "Get";

        /// <summary>
        /// The string for the Set method.
        /// </summary>
        public const string Set = "Set";

        /// <summary>
        /// The string for the Test method.
        /// </summary>
        public const string Test = "Test";

        /// <summary>
        /// Initializes a new instance of the <see cref="InvokeDscResourceException"/> class.
        /// Use this constructor when no error is generated by the invoke and the result is not a valid value.
        /// </summary>
        /// <param name="method">Method.</param>
        /// <param name="resourceName">Resource name.</param>
        /// <param name="module">Optional module.</param>
        public InvokeDscResourceException(string method, string resourceName, ModuleSpecification? module = null)
            : base(CreateMessage(method, resourceName, module, null))
        {
            // No message means that the invoke returned an invalid result.
            this.HResult = ErrorCodes.WinGetConfigUnitInvokeInvalidResult;
            this.Method = method;
            this.ResourceName = resourceName;
            this.Module = module;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="InvokeDscResourceException"/> class.
        /// Use this constructor when there is a message and the result is not valid.
        /// </summary>
        /// <param name="method">Method.</param>
        /// <param name="resourceName">Resource name.</param>
        /// <param name="message">Message.</param>
        public InvokeDscResourceException(string method, string resourceName, string message)
            : base(CreateMessage(method, resourceName, null, message))
        {
            this.HResult = ErrorCodes.WinGetConfigUnitInvokeInvalidResult;
            this.Method = method;
            this.ResourceName = resourceName;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="InvokeDscResourceException"/> class.
        /// Use this constructor when the invoke fails with an error message.
        /// </summary>
        /// <param name="method">Method.</param>
        /// <param name="resourceName">Resource name.</param>
        /// <param name="module">Optional module.</param>
        /// <param name="message">Message.</param>
        /// <param name="configurationSetSource">If true, the source of this error is set to be the configuration.</param>
        public InvokeDscResourceException(string method, string resourceName, ModuleSpecification? module, string message, bool configurationSetSource = false)
            : base(CreateMessage(method, resourceName, module, message))
        {
            this.HResult = GetHRForMethod(method);
            this.Method = method;
            this.ResourceName = resourceName;
            this.Module = module;
            this.Description = message;

            if (configurationSetSource)
            {
                this.ResultSource = ConfigurationUnitResultSource.ConfigurationSet;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="InvokeDscResourceException"/> class.
        /// Use this constructor when the invoke fails with an exception.
        /// </summary>
        /// <param name="method">Method.</param>
        /// <param name="resourceName">Resource name.</param>
        /// <param name="module">Optional module.</param>
        /// <param name="inner">The invoke exception.</param>
        public InvokeDscResourceException(string method, string resourceName, ModuleSpecification? module, Exception inner)
            : base(CreateMessage(method, resourceName, module, inner.Message), inner)
        {
            this.HResult = GetHRForMethod(method);
            this.Method = method;
            this.ResourceName = resourceName;
            this.Module = module;
            this.Description = (inner as RuntimeException)?.ErrorRecord.ToString() ?? inner.Message;
        }

        /// <summary>
        /// Gets the invoke method.
        /// </summary>
        public string Method { get; }

        /// <summary>
        /// Gets the resource name.
        /// </summary>
        public string ResourceName { get; }

        /// <summary>
        /// Gets the module, if any.
        /// </summary>
        public ModuleSpecification? Module { get; }

        /// <summary>
        /// Gets a value indicating the source of the result.
        /// </summary>
        public ConfigurationUnitResultSource ResultSource { get; } = ConfigurationUnitResultSource.UnitProcessing;

        /// <summary>
        /// Gets the description of the result.
        /// </summary>
        public string Description { get; } = string.Empty;

        /// <summary>
        /// Gets the details for the result.
        /// </summary>
        public string Details
        {
            get
            {
                RuntimeException? re = this.InnerException as RuntimeException;
                if (re != null)
                {
                    return re.ErrorRecord.ScriptStackTrace;
                }

                return this.ToString();
            }
        }

        /// <summary>
        /// Gets the HRESULT value for the given method.
        /// </summary>
        /// <param name="method">The method.</param>
        /// <returns>The HRESULT for the method.</returns>
        private static int GetHRForMethod(string method)
        {
            switch (method)
            {
                case Get: return ErrorCodes.WinGetConfigUnitInvokeGet;
                case Set: return ErrorCodes.WinGetConfigUnitInvokeSet;
                case Test: return ErrorCodes.WinGetConfigUnitInvokeTest;
            }

            return ErrorCodes.Unexpected;
        }

        private static string CreateMessage(string method, string resourceName, ModuleSpecification? module, string? message)
        {
            string result = $"Failed when calling `{method}` for resource: {resourceName} [{module?.ToString() ?? "<no module>"}]";
            if (message != null)
            {
                result += $" Message: '{message}'";
            }

            return result;
        }
    }
}
