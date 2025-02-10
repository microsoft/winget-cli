// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitProcessorBase.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Unit
{
    using System;
    using System.ComponentModel;
    using System.Runtime.CompilerServices;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.Processor.Factory;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Windows.Foundation.Collections;

    /// <summary>
    /// IConfigurationUnitProcessor base implementation.
    /// </summary>
    internal abstract partial class ConfigurationUnitProcessorBase
    {
        private readonly ConfigurationUnitInternal unitInternal;
        private readonly bool isLimitMode;

        private bool isTestInvoked = false;
        private bool isApplyInvoked = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationUnitProcessorBase"/> class.
        /// </summary>
        /// <param name="unitInternal">UnitInternal.</param>
        /// <param name="isLimitMode">Whether it is under limit mode.</param>
        internal ConfigurationUnitProcessorBase(ConfigurationUnitInternal unitInternal, bool isLimitMode)
        {
            this.unitInternal = unitInternal;
            this.isLimitMode = isLimitMode;
        }

        /// <summary>
        /// Gets the configuration unit that the processor was created for.
        /// </summary>
        public ConfigurationUnit Unit => this.unitInternal.Unit;

        /// <summary>
        /// Gets or initializes the set processor factory.
        /// </summary>
        internal ConfigurationSetProcessorFactoryBase? SetProcessorFactory { get; init; }

        /// <summary>
        /// Gets the current system state for the configuration unit.
        /// Calls Get on the DSC resource.
        /// </summary>
        /// <returns>A <see cref="IGetSettingsResult"/>.</returns>
        public IGetSettingsResult GetSettings()
        {
            this.OnDiagnostics(DiagnosticLevel.Verbose, $"Invoking `Get` for resource: {this.unitInternal.QualifiedName}...");

            this.CheckLimitMode(ConfigurationUnitIntent.Inform);
            var result = new GetSettingsResult(this.Unit);

            try
            {
                result.Settings = this.GetSettingsInternal();
            }
            catch (Exception e)
            {
                this.ExtractExceptionInformation(e, result.InternalResult);
            }

            this.OnDiagnostics(DiagnosticLevel.Verbose, $"... done invoking `Get`.");
            return result;
        }

        /// <summary>
        /// Determines if the system is already in the state described by the configuration unit.
        /// Calls Test on the DSC resource.
        /// </summary>
        /// <returns>A <see cref="ITestSettingsResult"/>.</returns>
        public ITestSettingsResult TestSettings()
        {
            this.OnDiagnostics(DiagnosticLevel.Verbose, $"Invoking `Test` for resource: {this.unitInternal.QualifiedName}...");

            if (this.Unit.Intent == ConfigurationUnitIntent.Inform)
            {
                this.OnDiagnostics(DiagnosticLevel.Error, "`Test` should not be called on a unit with intent of `Inform`");
                throw new NotSupportedException();
            }

            this.CheckLimitMode(ConfigurationUnitIntent.Assert);
            var result = new TestSettingsResult(this.Unit);
            result.TestResult = ConfigurationTestResult.Failed;
            try
            {
                bool testResult = this.TestSettingsInternal();

                result.TestResult = testResult ? ConfigurationTestResult.Positive : ConfigurationTestResult.Negative;
            }
            catch (Exception e)
            {
                this.ExtractExceptionInformation(e, result.InternalResult);
            }

            this.OnDiagnostics(DiagnosticLevel.Verbose, $"... done invoking `Test`.");
            return result;
        }

        /// <summary>
        /// Applies the state described in the configuration unit.
        /// Calls Set in the DSC resource.
        /// </summary>
        /// <returns>A <see cref="IApplySettingsResult"/>.</returns>
        public IApplySettingsResult ApplySettings()
        {
            this.OnDiagnostics(DiagnosticLevel.Verbose, $"Invoking `Apply` for resource: {this.unitInternal.QualifiedName}...");

            if (this.Unit.Intent == ConfigurationUnitIntent.Inform ||
                this.Unit.Intent == ConfigurationUnitIntent.Assert)
            {
                this.OnDiagnostics(DiagnosticLevel.Error, $"`Apply` should not be called on a unit with intent of `{this.Unit.Intent}`");
                throw new NotSupportedException();
            }

            this.CheckLimitMode(ConfigurationUnitIntent.Apply);
            var result = new ApplySettingsResult(this.Unit);
            try
            {
                result.RebootRequired = this.ApplySettingsInternal();
            }
            catch (Exception e)
            {
                this.ExtractExceptionInformation(e, result.InternalResult);
            }

            this.OnDiagnostics(DiagnosticLevel.Verbose, $"... done invoking `Apply`.");
            return result;
        }

        /// <summary>
        /// Gets the current settings.
        /// </summary>
        /// <returns>The current settings.</returns>
        protected abstract ValueSet GetSettingsInternal();

        /// <summary>
        /// Tests the current settings.
        /// </summary>
        /// <returns>A boolean indicating whether the settings are in the desired state.</returns>
        protected abstract bool TestSettingsInternal();

        /// <summary>
        /// Applies the desired settings.
        /// </summary>
        /// <returns>A boolean indicating whether a reboot is required.</returns>
        protected abstract bool ApplySettingsInternal();

        private void ExtractExceptionInformation(Exception e, ConfigurationUnitResultInformation resultInformation)
        {
            this.OnDiagnostics(DiagnosticLevel.Verbose, e.ToString());

            IConfigurationUnitResultException? configurationUnitResultException = e as IConfigurationUnitResultException;
            if (configurationUnitResultException != null)
            {
                resultInformation.ResultCode = e;
                resultInformation.Description = configurationUnitResultException.Description;
                resultInformation.Details = configurationUnitResultException.Details;
                resultInformation.ResultSource = configurationUnitResultException.ResultSource;
            }
            else
            {
                var inner = e.GetMostInnerException();
                resultInformation.ResultCode = inner;
                resultInformation.Description = e.Message;
                resultInformation.Details = e.ToString();
                resultInformation.ResultSource = ConfigurationUnitResultSource.Internal;
            }
        }

        [MethodImpl(MethodImplOptions.Synchronized)]
        private void CheckLimitMode(ConfigurationUnitIntent intent)
        {
            if (!this.isLimitMode)
            {
                return;
            }

            if (intent == ConfigurationUnitIntent.Unknown)
            {
                throw new InvalidEnumArgumentException(nameof(ConfigurationUnitIntent.Unknown));
            }

            if (intent == ConfigurationUnitIntent.Assert)
            {
                if (this.isTestInvoked)
                {
                    this.OnDiagnostics(DiagnosticLevel.Error, "TestSettings is already invoked in limit mode.");
                    throw new InvalidOperationException("TestSettings is already invoked in limit mode.");
                }
                else
                {
                    this.isTestInvoked = true;
                }
            }

            if (intent == ConfigurationUnitIntent.Apply)
            {
                if (this.isApplyInvoked)
                {
                    this.OnDiagnostics(DiagnosticLevel.Error, "ApplySettings is already invoked in limit mode.");
                    throw new InvalidOperationException("ApplySettings is already invoked in limit mode.");
                }
                else
                {
                    this.isApplyInvoked = true;
                }
            }

            // Get is always allowed now.
        }

        private void OnDiagnostics(DiagnosticLevel level, string message)
        {
            this.SetProcessorFactory?.OnDiagnostics(level, message);
        }
    }
}
