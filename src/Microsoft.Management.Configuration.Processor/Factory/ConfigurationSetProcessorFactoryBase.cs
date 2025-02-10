// -----------------------------------------------------------------------------
// <copyright file="ConfigurationSetProcessorFactoryBase.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Factory
{
    using System;
    using System.Runtime.CompilerServices;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.Set;

    /// <summary>
    /// IConfigurationSetProcessorFactory base implementation.
    /// </summary>
    internal abstract partial class ConfigurationSetProcessorFactoryBase
    {
        private bool isCreateProcessorInvoked = false;

        // Backing variables for properties that are restricted in limit mode.
        private ConfigurationSet? limitationSet;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationSetProcessorFactoryBase"/> class.
        /// </summary>
        public ConfigurationSetProcessorFactoryBase()
        {
        }

        /// <summary>
        /// Diagnostics event; useful for logging and/or verbose output.
        /// </summary>
        public event EventHandler<IDiagnosticInformation>? Diagnostics;

        /// <summary>
        /// Gets or sets the minimum diagnostic level to send.
        /// </summary>
        public DiagnosticLevel MinimumLevel { get; set; } = DiagnosticLevel.Informational;

        /// <summary>
        /// Gets or sets the limitation set. Limitation set can only be set once.
        /// </summary>
        public ConfigurationSet? LimitationSet
        {
            get
            {
                return this.limitationSet;
            }

            set
            {
                if (this.IsLimitMode())
                {
                    throw new InvalidOperationException("Setting LimitationSet in limit mode is invalid.");
                }

                this.limitationSet = value;
            }
        }

        /// <summary>
        /// Gets the configuration unit processor details for the given unit.
        /// </summary>
        /// <param name="incomingSet">Configuration Set.</param>
        /// <returns>Configuration set processor.</returns>
        public IConfigurationSetProcessor CreateSetProcessor(ConfigurationSet? incomingSet)
        {
            try
            {
                bool isLimitMode = this.IsLimitMode();
                this.OnDiagnostics(DiagnosticLevel.Informational, $"The set processor factory is running in limit mode: {isLimitMode}.");

                this.CheckLimitMode();

                ConfigurationSet? set = isLimitMode ? this.limitationSet : incomingSet;

                this.OnDiagnostics(DiagnosticLevel.Verbose, $"Creating set processor for `{set?.Name ?? "<null>"}`...");

                if (set != null && (set.Parameters.Count > 0 || set.Variables.Count > 0))
                {
                    this.OnDiagnostics(DiagnosticLevel.Error, $"  Parameters/variables are not yet supported.");
                    throw new NotImplementedException();
                }

                IConfigurationSetProcessor result = this.CreateSetProcessorInternal(set, isLimitMode);

                this.OnDiagnostics(DiagnosticLevel.Verbose, "... done creating set processor.");

                return result;
            }
            catch (Exception ex)
            {
                this.OnDiagnostics(DiagnosticLevel.Error, ex.ToString());
                throw;
            }
        }

        /// <summary>
        /// Sends diagnostics if appropriate.
        /// </summary>
        /// <param name="level">The level of this diagnostic message.</param>
        /// <param name="message">The diagnostic message.</param>
        internal void OnDiagnostics(DiagnosticLevel level, string message)
        {
            EventHandler<IDiagnosticInformation>? diagnostics = this.Diagnostics;
            if (diagnostics != null && level >= this.MinimumLevel)
            {
                this.InvokeDiagnostics(diagnostics, level, message);
            }
        }

        /// <summary>
        /// Determines if diagnostics are enabled.
        /// This allows optimizing out string construction for some cases.
        /// </summary>
        /// <returns>True if diagnostics are enabled; false if not.</returns>
        protected bool AreDiagnosticsEnabled()
        {
            return this.Diagnostics != null;
        }

        /// <summary>
        /// Gets the configuration unit processor details for the given unit.
        /// </summary>
        /// <param name="set">Configuration Set.</param>
        /// <param name="isLimitMode">Whether the processor should be in limit mode.</param>
        /// <returns>Configuration set processor.</returns>
        protected abstract IConfigurationSetProcessor CreateSetProcessorInternal(ConfigurationSet? set, bool isLimitMode);

        /// <summary>
        /// Gets a value indicating whether the factory is operation in limit mode.
        /// </summary>
        /// <returns>True if the factory is in limit mode, false otherwise.</returns>
        protected bool IsLimitMode()
        {
            return this.limitationSet != null;
        }

        /// <summary>
        /// Sends diagnostics to the given handler.
        /// </summary>
        /// <param name="diagnostics">The handler to invoke.</param>
        /// <param name="level">The level of this diagnostic message.</param>
        /// <param name="message">The diagnostic message.</param>
        private void InvokeDiagnostics(EventHandler<IDiagnosticInformation> diagnostics, DiagnosticLevel level, string message)
        {
            Helpers.DiagnosticInformation information = new ()
            {
                Level = level,
                Message = message,
            };
            diagnostics.Invoke(this, information);
        }

        [MethodImpl(MethodImplOptions.Synchronized)]
        private void CheckLimitMode()
        {
            if (!this.IsLimitMode())
            {
                return;
            }

            if (this.isCreateProcessorInvoked)
            {
                this.OnDiagnostics(DiagnosticLevel.Error, "CreateSetProcessor is already invoked in limit mode.");
                throw new InvalidOperationException("CreateSetProcessor is already invoked in limit mode.");
            }
            else
            {
                this.isCreateProcessorInvoked = true;
            }
        }
    }
}
