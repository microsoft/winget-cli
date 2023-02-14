// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Unit
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.PowerShell.Commands;

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
        /// Gets the directives overlay that the processor was created with.
        /// </summary>
        public IReadOnlyDictionary<string, object>? DirectivesOverlay => this.unitResource.DirectivesOverlay;

        /// <summary>
        /// Gets the current system state for the configuration unit.
        /// Calls Get on the DSC resource.
        /// </summary>
        /// <returns>A <see cref="GetSettingsResult"/>.</returns>
        public GetSettingsResult GetSettings()
        {
            var result = new GetSettingsResult();
            try
            {
                result.Settings = this.processorEnvironment.InvokeGetResource(
                    this.unitResource.GetExpandedSettings(),
                    this.unitResource.ResourceName,
                    this.unitResource.Module);
            }
            catch (Exception e) when (e is RuntimeException ||
                                      e is WriteErrorException)
            {
                var inner = e.GetMostInnerException();
                result.ResultInformation.ResultCode = inner;
                result.ResultInformation.Description = e.ToString();
            }
            catch (Exception)
            {
                throw;
            }

            return result;
        }

        /// <summary>
        /// Determines if the system is already in the state described by the configuration unit.
        /// Calls Test on the DSC resource.
        /// </summary>
        /// <returns>A <see cref="TestSettingsResult"/>.</returns>
        public TestSettingsResult TestSettings()
        {
            if (this.Unit.Intent == ConfigurationUnitIntent.Inform)
            {
                throw new NotSupportedException();
            }

            var result = new TestSettingsResult();
            result.TestResult = ConfigurationTestResult.Failed;
            try
            {
                bool testResult = this.processorEnvironment.InvokeTestResource(
                    this.unitResource.GetExpandedSettings(),
                    this.unitResource.ResourceName,
                    this.unitResource.Module);

                result.TestResult = testResult ? ConfigurationTestResult.Positive : ConfigurationTestResult.Negative;
            }
            catch (Exception e) when (e is RuntimeException ||
                                      e is WriteErrorException)
            {
                var inner = e.GetMostInnerException();
                result.ResultInformation.ResultCode = inner;
                result.ResultInformation.Description = e.ToString();
            }
            catch (Exception)
            {
                throw;
            }

            return result;
        }

        /// <summary>
        /// Applies the state described in the configuration unit.
        /// Calls Set in the DSC resource.
        /// </summary>
        /// <returns>A <see cref="ApplySettingsResult"/>.</returns>
        public ApplySettingsResult ApplySettings()
        {
            if (this.Unit.Intent == ConfigurationUnitIntent.Inform ||
                this.Unit.Intent == ConfigurationUnitIntent.Assert)
            {
                throw new NotSupportedException();
            }

            var result = new ApplySettingsResult();
            try
            {
                result.RebootRequired = this.processorEnvironment.InvokeSetResource(
                                            this.unitResource.GetExpandedSettings(),
                                            this.unitResource.ResourceName,
                                            this.unitResource.Module);
            }
            catch (Exception e) when (e is RuntimeException ||
                                      e is WriteErrorException)
            {
                var inner = e.GetMostInnerException();
                result.ResultInformation.ResultCode = inner;
                result.ResultInformation.Description = e.ToString();
            }
            catch (Exception)
            {
                throw;
            }

            return result;
        }
    }
}
