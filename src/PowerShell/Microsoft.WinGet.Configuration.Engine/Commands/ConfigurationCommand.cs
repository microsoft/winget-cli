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
    using Microsoft.WinGet.Configuration.Engine.PSObjects;
    using Windows.Storage;
    using Windows.Storage.Streams;

    /// <summary>
    /// Class that deals configuration commands.
    /// </summary>
    public sealed class ConfigurationCommand : MtaCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        public ConfigurationCommand(PSCmdlet psCmdlet, CancellationToken cancellationToken)
            : base(psCmdlet, cancellationToken)
        {
        }

        /// <summary>
        /// Open a configuration set.
        /// </summary>
        /// <param name="configFile">Configuration file path.</param>
        public void Get(string configFile)
        {
            if (!Path.IsPathRooted(configFile))
            {
                configFile = Path.GetFullPath(
                    Path.Combine(
                        this.PsCmdlet.SessionState.Path.CurrentFileSystemLocation.Path,
                        configFile));
            }

            if (!File.Exists(configFile))
            {
                throw new FileNotFoundException(configFile);
            }

            // Start task.
            var runningTask = this.RunOnMTA<PSConfigurationSet>(
                async () => await this.OpenConfigurationSetAsync(configFile));

            this.Wait(runningTask);
            this.WriteObject(runningTask.Result);
        }

        /// <summary>
        /// Starts configuration.
        /// </summary>
        /// <param name="psConfigurationSet">PSConfigurationSet.</param>
        /// <param name="agrementsAccepted">If the user accepts the configuration agreements.</param>
        public void Start(PSConfigurationSet psConfigurationSet, bool agrementsAccepted)
        {
            // TODO: confirm
            if (!psConfigurationSet.HasDetailsRetrieved)
            {
                this.WriteDebug("Getting configuration set");
                var runningTask = this.RunOnMTA<PSConfigurationSet>(
                    async () => await this.GetSetDetailsAsync(psConfigurationSet));

                // TODO: remove this.
                this.Wait(runningTask);
            }

            // TODO: apply.
        }

        private ConfigurationProcessor CreateConfigurationProcessor()
        {
            var factory = new ConfigurationSetProcessorFactory(
                ConfigurationProcessorType.Default, null);

            var processor = new ConfigurationProcessor(factory);

            // TODO: set up logging and telemetry.
            ////processor.MinimumLevel = DiagnosticLevel.Error;
            ////processor.Caller = "ConfigurationCmdlet";
            ////processor.ActivityIdentifier = Guid.NewGuid();
            ////processor.GenerateTelemetryEvents = false;
            ////processor.Diagnostics;

            return processor;
        }

        private async Task<PSConfigurationSet> OpenConfigurationSetAsync(string configFile)
        {
            var processor = this.CreateConfigurationProcessor();

            var stream = await FileRandomAccessStream.OpenAsync(configFile, FileAccessMode.Read);
            OpenConfigurationSetResult openResult = await processor.OpenConfigurationSetAsync(stream);
            if (openResult.Set is null)
            {
                // TODO: throw better exception.
                throw new Exception($"Failed opening configuration set. Result 0x{openResult.ResultCode} at {openResult.Field}");
            }

            var set = openResult.Set;

            // This should match winget's OpenConfigurationSet or OpenConfigurationSetAsync
            // should be modify to take the full path and handle it.
            set.Name = Path.GetFileName(configFile);
            set.Origin = Path.GetDirectoryName(configFile);
            set.Path = configFile;

            return new PSConfigurationSet(processor, set);
        }

        private async Task<PSConfigurationSet> GetSetDetailsAsync(PSConfigurationSet psConfigurationSet)
        {
            var processor = psConfigurationSet.Processor;
            var set = psConfigurationSet.Set;

            if (set.ConfigurationUnits.Count == 0)
            {
                this.WriteWarning("Configuration File Empty");
            }

            // TODO: implement progress
            var result = await processor.GetSetDetailsAsync(set, ConfigurationUnitDetailLevel.Catalog);

            psConfigurationSet.HasDetailsRetrieved = true;
            return psConfigurationSet;
        }
    }
}
