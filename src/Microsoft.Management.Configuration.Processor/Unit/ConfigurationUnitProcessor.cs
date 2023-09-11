﻿// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Unit
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;

    /// <summary>
    /// Provides access to a specific configuration unit within the runtime.
    /// </summary>
    internal sealed class ConfigurationUnitProcessor : IConfigurationUnitProcessor
    {
        private readonly IProcessorEnvironment processorEnvironment;
        private readonly ConfigurationUnitAndResource unitResource;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationUnitProcessor"/> class.
        /// </summary>
        /// <param name="processorEnvironment">Processor environment.</param>
        /// <param name="unitResource">UnitResource.</param>
        internal ConfigurationUnitProcessor(
            IProcessorEnvironment processorEnvironment,
            ConfigurationUnitAndResource unitResource)
        {
            this.processorEnvironment = processorEnvironment;
            this.unitResource = unitResource;
        }

        /// <summary>
        /// Gets the configuration unit that the processor was created for.
        /// </summary>
        public ConfigurationUnit Unit => this.unitResource.Unit;

        /// <summary>
        /// Gets or initializes the set processor factory.
        /// </summary>
        internal PowerShellConfigurationSetProcessorFactory? SetProcessorFactory { get; init; }

        /// <summary>
        /// Gets the current system state for the configuration unit.
        /// Calls Get on the DSC resource.
        /// </summary>
        /// <returns>A <see cref="IGetSettingsResult"/>.</returns>
        public IGetSettingsResult GetSettings()
        {
            this.OnDiagnostics(DiagnosticLevel.Verbose, $"Invoking `Get` for resource: {this.unitResource.UnitInternal.ToIdentifyingString()}...");

            var result = new GetSettingsResult(this.Unit);

            try
            {
                result.Settings = this.processorEnvironment.InvokeGetResource(
                    this.unitResource.GetSettings(),
                    this.unitResource.ResourceName,
                    this.unitResource.Module);
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
            this.OnDiagnostics(DiagnosticLevel.Verbose, $"Invoking `Test` for resource: {this.unitResource.UnitInternal.ToIdentifyingString()}...");

            if (this.Unit.Intent == ConfigurationUnitIntent.Inform)
            {
                this.OnDiagnostics(DiagnosticLevel.Error, "`Test` should not be called on a unit with intent of `Inform`");
                throw new NotSupportedException();
            }

            var result = new TestSettingsResult(this.Unit);
            result.TestResult = ConfigurationTestResult.Failed;
            try
            {
                bool testResult = this.processorEnvironment.InvokeTestResource(
                    this.unitResource.GetSettings(),
                    this.unitResource.ResourceName,
                    this.unitResource.Module);

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
            this.OnDiagnostics(DiagnosticLevel.Verbose, $"Invoking `Apply` for resource: {this.unitResource.UnitInternal.ToIdentifyingString()}...");

            if (this.Unit.Intent == ConfigurationUnitIntent.Inform ||
                this.Unit.Intent == ConfigurationUnitIntent.Assert)
            {
                this.OnDiagnostics(DiagnosticLevel.Error, $"`Apply` should not be called on a unit with intent of `{this.Unit.Intent}`");
                throw new NotSupportedException();
            }

            var result = new ApplySettingsResult(this.Unit);
            try
            {
                result.RebootRequired = this.processorEnvironment.InvokeSetResource(
                                            this.unitResource.GetSettings(),
                                            this.unitResource.ResourceName,
                                            this.unitResource.Module);
            }
            catch (Exception e)
            {
                this.ExtractExceptionInformation(e, result.InternalResult);
            }

            this.OnDiagnostics(DiagnosticLevel.Verbose, $"... done invoking `Apply`.");
            return result;
        }

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

        private void OnDiagnostics(DiagnosticLevel level, string message)
        {
            this.SetProcessorFactory?.OnDiagnostics(level, message);
        }
    }
}
