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
    using Microsoft.PowerShell;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;
    using Windows.Storage;
    using Windows.Storage.Streams;

    /// <summary>
    /// Class that deals configuration commands.
    /// </summary>
    public sealed class ConfigurationCommand : AsyncCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <param name="canWriteToStream">If the command can write to stream.</param>
        public ConfigurationCommand(PSCmdlet psCmdlet, CancellationToken cancellationToken, bool canWriteToStream = true)
            : base(psCmdlet, cancellationToken, canWriteToStream)
        {
        }

        /// <summary>
        /// Open a configuration set.
        /// </summary>
        /// <param name="configFile">Configuration file path.</param>
        /// <param name="executionPolicy">Execution policy.</param>
        public void Get(string configFile, ExecutionPolicy executionPolicy)
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
                async () => await this.OpenConfigurationSetAsync(configFile, executionPolicy));

            this.Wait(runningTask);
            this.WriteObject(runningTask.Result);
        }

        /// <summary>
        /// Gets the details of a configuration set.
        /// </summary>
        /// <param name="psConfigurationSet">PSConfigurationSet.</param>
        public void GetDetails(PSConfigurationSet psConfigurationSet)
        {
            if (!psConfigurationSet.HasDetails)
            {
                if (!psConfigurationSet.CanProcess())
                {
                    // TODO: better exception or just write info and return null.
                    throw new Exception("Someone is using me!!!");
                }

                var runningTask = this.RunOnMTA<PSConfigurationSet>(
                async () =>
                {
                    try
                    {
                        psConfigurationSet = await this.GetSetDetailsAsync(psConfigurationSet);
                    }
                    finally
                    {
                        psConfigurationSet.DoneProcessing();
                    }

                    return psConfigurationSet;
                });

                this.Wait(runningTask);
                psConfigurationSet = runningTask.Result;
            }
            else
            {
                this.WriteWarning("Details already obtained for this set");
            }

            this.WriteObject(psConfigurationSet);
        }

        /// <summary>
        /// Starts configuration.
        /// </summary>
        /// <param name="psConfigurationSet">PSConfigurationSet.</param>
        public void StartApply(PSConfigurationSet psConfigurationSet)
        {
            if (psConfigurationSet.Set.State == ConfigurationSetState.Completed)
            {
                this.WriteWarning("Processing this set is completed");
                return;
            }

            if (!psConfigurationSet.CanProcess())
            {
                // TODO: better exception or just write info and return null.
                throw new Exception("Someone is using me!!!");
            }

            var configurationJob = this.StartApplyInternal(psConfigurationSet);
            this.WriteObject(configurationJob);
        }

        /// <summary>
        /// Applies configuration.
        /// </summary>
        /// <param name="psConfigurationSet">PSConfigurationSet.</param>
        public void Apply(PSConfigurationSet psConfigurationSet) => this.ContinueHelper(this.StartApplyInternal(psConfigurationSet));

        /// <summary>
        /// Continue a configuration job.
        /// </summary>
        /// <param name="psConfigurationJob">The configuration job.</param>
        public void Continue(PSConfigurationJob psConfigurationJob)
        {
            if (psConfigurationJob.ConfigurationTask.IsCompleted)
            {
                this.WriteDebug("The task was completed before waiting");
                if (psConfigurationJob.ConfigurationTask.IsCompletedSuccessfully)
                {
                    this.WriteDebug("Completed successfully");
                    this.WriteObject(psConfigurationJob.ConfigurationTask.Result);
                    return;
                }
                else if (psConfigurationJob.ConfigurationTask.IsFaulted)
                {
                    this.WriteDebug("Completed faulted before waiting");

                    // Maybe just write error?
                    throw psConfigurationJob.ConfigurationTask.Exception!;
                }
            }

            this.ContinueHelper(psConfigurationJob);
        }

        private void ContinueHelper(PSConfigurationJob psConfigurationJob)
        {
            // Signal the command that it can write to streams and wait for task.
            this.WriteDebug("Waiting for task to complete");
            psConfigurationJob.StartCommand.Wait(psConfigurationJob.ConfigurationTask);
            this.WriteObject(psConfigurationJob.ConfigurationTask.Result);
        }

        private ConfigurationProcessor CreateConfigurationProcessor(ExecutionPolicy executionPolicy)
        {
            var properties = new ConfigurationProcessorFactoryProperties();
            properties.Policy = this.GetConfigurationProcessorPolicy(executionPolicy);

            var factory = new ConfigurationSetProcessorFactory(
                ConfigurationProcessorType.Default, properties);

            var processor = new ConfigurationProcessor(factory);

            // TODO: set up logging and telemetry.
            ////processor.MinimumLevel = DiagnosticLevel.Error;
            ////processor.Caller = "ConfigurationModule";
            ////processor.ActivityIdentifier = Guid.NewGuid();
            ////processor.GenerateTelemetryEvents = false;
            ////processor.Diagnostics;

            return processor;
        }

        private async Task<PSConfigurationSet> OpenConfigurationSetAsync(string configFile, ExecutionPolicy executionPolicy)
        {
            var processor = this.CreateConfigurationProcessor(executionPolicy);

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

        private PSConfigurationJob StartApplyInternal(PSConfigurationSet psConfigurationSet)
        {
            var runningTask = this.RunOnMTA<PSConfigurationSet>(
                async () =>
                {
                    try
                    {
                        psConfigurationSet = await this.ApplyConfigurationAsync(psConfigurationSet);
                    }
                    finally
                    {
                        psConfigurationSet.DoneProcessing();
                    }

                    return psConfigurationSet;
                });

            return new PSConfigurationJob(runningTask, this);
        }

        private async Task<PSConfigurationSet> ApplyConfigurationAsync(PSConfigurationSet psConfigurationSet)
        {
            if (!psConfigurationSet.HasDetails)
            {
                this.WriteDebug("Getting details for configuration set");
                await this.GetSetDetailsAsync(psConfigurationSet);
            }

            var processor = psConfigurationSet.Processor;
            var set = psConfigurationSet.Set;

            // TODO: implement progress
            _ = await processor.ApplySetAsync(set, ApplyConfigurationSetFlags.None);

            return psConfigurationSet;
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
            _ = await processor.GetSetDetailsAsync(set, ConfigurationUnitDetailLevel.Catalog);

            psConfigurationSet.HasDetails = true;
            return psConfigurationSet;
        }

        private ConfigurationProcessorPolicy GetConfigurationProcessorPolicy(ExecutionPolicy policy)
        {
            return policy switch
            {
                ExecutionPolicy.Unrestricted => ConfigurationProcessorPolicy.Unrestricted,
                ExecutionPolicy.RemoteSigned => ConfigurationProcessorPolicy.RemoteSigned,
                ExecutionPolicy.AllSigned => ConfigurationProcessorPolicy.AllSigned,
                ExecutionPolicy.Restricted => ConfigurationProcessorPolicy.Restricted,
                ExecutionPolicy.Bypass => ConfigurationProcessorPolicy.Bypass,
                _ => throw new InvalidOperationException(),
            };
        }
    }
}
