// -----------------------------------------------------------------------------
// <copyright file="ConfigurationCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Commands
{
    using System;
    using System.IO;
    using System.Management.Automation;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor;
    using Windows.Storage;
    using Windows.Storage.Streams;

    /// <summary>
    /// Class that deals with start and invoke the configuration.
    /// </summary>
    public sealed class ConfigurationCommand : MtaCommand
    {
        private readonly string configFile;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <param name="configFile">Configuration file path.</param>
        public ConfigurationCommand(PSCmdlet psCmdlet, CancellationToken cancellationToken, string configFile)
            : base(psCmdlet, cancellationToken)
        {
            if (!File.Exists(configFile))
            {
                throw new FileNotFoundException(configFile);
            }

            this.configFile = configFile;
        }

        /// <summary>
        /// Invoke configuration. Waits until completed.
        /// </summary>
        public void Invoke()
        {
            // Start task.
            var runningTask = this.RunOnMTA(this.InvokeAsync);

            // Wait for it to complete or being cancelled.
            this.Wait(runningTask);
        }

        private async Task InvokeAsync()
        {
            if (Thread.CurrentThread.GetApartmentState() != ApartmentState.MTA)
            {
                throw new NotSupportedException("Calling from an STA");
            }

            // This will fail with E_NOTIMPL
            var factory = new ConfigurationSetProcessorFactory(
                ConfigurationProcessorType.Default, null);
            var processor = new ConfigurationProcessor(factory);

            var configSet = await this.CreateConfigurationSetAsync(processor);
            _ = await processor.GetSetDetailsAsync(configSet, ConfigurationUnitDetailLevel.Catalog);
        }

        private async Task<ConfigurationSet> CreateConfigurationSetAsync(ConfigurationProcessor processor)
        {
            var stream = await FileRandomAccessStream.OpenAsync(this.configFile, FileAccessMode.Read);

            OpenConfigurationSetResult openResult = await processor.OpenConfigurationSetAsync(stream);

            if (openResult.Set is null)
            {
                // TODO: throw better exception.
                throw new Exception($"Failed opening configuration set. Result 0x{openResult.ResultCode} at {openResult.Field}");
            }

            return openResult.Set;
        }
    }
}
