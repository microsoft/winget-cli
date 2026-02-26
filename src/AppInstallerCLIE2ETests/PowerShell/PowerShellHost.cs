// -----------------------------------------------------------------------------
// <copyright file="PowerShellHost.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.PowerShell
{
    using System;
    using System.Collections;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using Microsoft.PowerShell;
    using NUnit.Framework;

    /// <summary>
    /// Helper class to run powershell commands.
    /// </summary>
    internal class PowerShellHost : IDisposable
    {
        private readonly Runspace runspace = null;

        private bool disposed = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="PowerShellHost"/> class.
        /// </summary>
        public PowerShellHost()
        {
            InitialSessionState initialSessionState = InitialSessionState.CreateDefault();
            initialSessionState.ExecutionPolicy = ExecutionPolicy.Unrestricted;

            this.runspace = RunspaceFactory.CreateRunspace(initialSessionState);
            this.runspace.Open();
            this.VerifyErrorState();

            this.PowerShell = PowerShell.Create(this.runspace);
        }

        /// <summary>
        /// Finalizes an instance of the <see cref="PowerShellHost"/> class.
        /// </summary>
        ~PowerShellHost() => this.Dispose(false);

        /// <summary>
        /// Gets PowerShell.
        /// </summary>
        public PowerShell PowerShell { get; private set; } = null;

        /// <summary>
        /// Dispose.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Add module path.
        /// </summary>
        /// <param name="path">Path.</param>
        public void AddModulePath(string path)
        {
            var newModulePath = this.PowerShell.Runspace.SessionStateProxy.PSVariable.GetValue("env:PSModulePath") + $";{path}";
            this.PowerShell.Runspace.SessionStateProxy.PSVariable.Set("env:PSModulePath", newModulePath);
        }

        /// <summary>
        /// Protected implementation of dispose pattern.
        /// </summary>
        /// <param name="disposing">Dispose.</param>
        protected virtual void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    this.PowerShell.Dispose();
                    this.runspace.Dispose();
                }

                this.disposed = true;
            }
        }

        /// <summary>
        /// The most common error is that the module was not found.
        /// </summary>
        private void VerifyErrorState()
        {
            var errors = (ArrayList)this.runspace.SessionStateProxy.PSVariable.GetValue("Error");

            if (errors.Count > 0)
            {
                string errorMessage = "PSVariable Error:";
                foreach (var error in errors)
                {
                    errorMessage += Environment.NewLine + ((ErrorRecord)error).Exception.Message;
                }

                TestContext.Error.WriteLine(errorMessage);
                throw new Exception(errorMessage);
            }
        }
    }
}
