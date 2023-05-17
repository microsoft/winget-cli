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
    using System.Threading.Tasks;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor;
    using Microsoft.PowerShell;
    using Microsoft.WinGet.Configuration.Engine.Exceptions;
    using Microsoft.WinGet.Configuration.Engine.Helpers;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;
    using Microsoft.WinGet.Configuration.Engine.Resources;
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
        public ConfigurationCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
        }

        /// <summary>
        /// Verify user accept agreements.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        /// <param name="hasAccepted">Has already accepted.</param>
        /// <returns>If accepted.</returns>
        public static bool ConfirmConfigurationProcessing(PSCmdlet psCmdlet, bool hasAccepted)
        {
            bool result = false;
            if (!hasAccepted)
            {
                bool yesToAll = false;
                bool noToAll = false;
                result = psCmdlet.ShouldContinue(Resources.ConfigurationWarningPrompt, Resources.ConfigurationWarning, true, ref yesToAll, ref noToAll);

                if (yesToAll)
                {
                    result = true;
                }
                else if (noToAll)
                {
                    result = false;
                }
            }
            else
            {
                // This way even if they set WarningActionPreference.Ignore we will still print the
                // warning message if the agreements didn't get accepted.
                psCmdlet.WriteWarning(Resources.ConfigurationWarning);
                result = true;
            }

            return result;
        }

        /// <summary>
        /// Open a configuration set.
        /// </summary>
        /// <param name="configFile">Configuration file path.</param>
        /// <param name="executionPolicy">Execution policy.</param>
        /// <param name="canUseTelemetry">If telemetry can be used.</param>
        public void Get(string configFile, ExecutionPolicy executionPolicy, bool canUseTelemetry)
        {
            configFile = this.VerifyFile(configFile);

            // Start task.
            var runningTask = this.RunOnMTA<PSConfigurationSet>(
                async () =>
                {
                    try
                    {
                        return await this.OpenConfigurationSetAsync(configFile, executionPolicy, canUseTelemetry);
                    }
                    finally
                    {
                        this.Complete();
                    }
                });

            this.Wait(runningTask);
            this.Write(StreamType.Object, runningTask.Result);
        }

        /// <summary>
        /// Gets the details of a configuration set.
        /// </summary>
        /// <param name="psConfigurationSet">PSConfigurationSet.</param>
        public void GetDetails(PSConfigurationSet psConfigurationSet)
        {
            psConfigurationSet.PsProcessor.UpdateDiagnosticCmdlet(this);

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
                        this.Complete();
                        psConfigurationSet.DoneProcessing();
                    }

                    return psConfigurationSet;
                });

                this.Wait(runningTask);
                psConfigurationSet = runningTask.Result;
            }
            else
            {
                this.Write(StreamType.Warning, "Details already obtained for this set");
            }

            this.Write(StreamType.Object, psConfigurationSet);
        }

        /// <summary>
        /// Starts configuration.
        /// </summary>
        /// <param name="psConfigurationSet">PSConfigurationSet.</param>
        public void StartApply(PSConfigurationSet psConfigurationSet)
        {
            if (psConfigurationSet.Set.State == ConfigurationSetState.Completed)
            {
                this.Write(StreamType.Warning, "Processing this set is completed");
                return;
            }

            if (!psConfigurationSet.CanProcess())
            {
                // TODO: better exception or just write info and return null.
                throw new Exception("Someone is using me!!!");
            }

            var configurationJob = this.StartApplyInternal(psConfigurationSet);
            this.Write(StreamType.Object, configurationJob);
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
                // It is safe to print all output.
                psConfigurationJob.StartCommand.ConsumeStreams();

                this.Write(StreamType.Verbose, "The task was completed before waiting");
                if (psConfigurationJob.ConfigurationTask.IsCompletedSuccessfully)
                {
                    this.Write(StreamType.Verbose, "Completed successfully");
                    this.Write(StreamType.Object, psConfigurationJob.ConfigurationTask.Result);
                    return;
                }
                else if (psConfigurationJob.ConfigurationTask.IsFaulted)
                {
                    this.Write(StreamType.Verbose, "Completed faulted before waiting");

                    // Maybe just write error?
                    throw psConfigurationJob.ConfigurationTask.Exception!;
                }
            }

            this.ContinueHelper(psConfigurationJob);
        }

        /// <summary>
        /// Verifies file exists and return the full path, if not already.
        /// </summary>
        /// <param name="filePath">File path.</param>
        /// <returns>Full path.</returns>
        private string VerifyFile(string filePath)
        {
            if (!Path.IsPathRooted(filePath))
            {
                filePath = Path.GetFullPath(
                    Path.Combine(
                        this.PsCmdlet.SessionState.Path.CurrentFileSystemLocation.Path,
                        filePath));
            }

            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException(filePath);
            }

            return filePath;
        }

        private void ContinueHelper(PSConfigurationJob psConfigurationJob)
        {
            // Signal the command that it can write to streams and wait for task.
            this.Write(StreamType.Verbose, "Waiting for task to complete");
            psConfigurationJob.StartCommand.Wait(psConfigurationJob.ConfigurationTask);
            this.Write(StreamType.Object, psConfigurationJob.ConfigurationTask.Result);
        }

        private PSConfigurationProcessor CreateConfigurationProcessor(ExecutionPolicy executionPolicy, bool canUseTelemetry)
        {
            this.Write(StreamType.Information, Resources.ConfigurationInitializing);

            var properties = new ConfigurationProcessorFactoryProperties();
            properties.Policy = this.GetConfigurationProcessorPolicy(executionPolicy);

            var factory = new ConfigurationSetProcessorFactory(
                ConfigurationProcessorType.Default, properties);

            return new PSConfigurationProcessor(factory, this, canUseTelemetry);
        }

        private async Task<PSConfigurationSet> OpenConfigurationSetAsync(string configFile, ExecutionPolicy executionPolicy, bool canUseTelemetry)
        {
            var psProcessor = this.CreateConfigurationProcessor(executionPolicy, canUseTelemetry);

            this.Write(StreamType.Information, Resources.ConfigurationReadingConfigFile);
            var stream = await FileRandomAccessStream.OpenAsync(configFile, FileAccessMode.Read);

            OpenConfigurationSetResult openResult = await psProcessor.Processor.OpenConfigurationSetAsync(stream);
            if (openResult.ResultCode != null)
            {
                throw new OpenConfigurationSetException(openResult, configFile);
            }

            var set = openResult.Set;

            // This should match winget's OpenConfigurationSet or OpenConfigurationSetAsync
            // should be modify to take the full path and handle it.
            set.Name = Path.GetFileName(configFile);
            set.Origin = Path.GetDirectoryName(configFile);
            set.Path = configFile;

            return new PSConfigurationSet(psProcessor, set);
        }

        private PSConfigurationJob StartApplyInternal(PSConfigurationSet psConfigurationSet)
        {
            psConfigurationSet.PsProcessor.UpdateDiagnosticCmdlet(this);

            var runningTask = this.RunOnMTA<PSConfigurationSet>(
                async () =>
                {
                    try
                    {
                        psConfigurationSet = await this.ApplyConfigurationAsync(psConfigurationSet);
                    }
                    finally
                    {
                        this.Complete();
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
                this.Write(StreamType.Verbose, "Getting details for configuration set");
                await this.GetSetDetailsAsync(psConfigurationSet);
            }

            var processor = psConfigurationSet.PsProcessor.Processor;
            var set = psConfigurationSet.Set;

            var applyProgressOutput = new ApplyConfigurationSetProgressOutput(
                this,
                this.GetNewProgressActivityId(),
                Resources.ConfigurationApply,
                Resources.OperationInProgress,
                Resources.OperationCompleted,
                set.ConfigurationUnits.Count);

            var applyTask = processor.ApplySetAsync(set, ApplyConfigurationSetFlags.None);
            applyTask.Progress = applyProgressOutput.Progress;

            try
            {
                var result = await applyTask;
                applyProgressOutput.HandleUnreportedProgress(result);
            }
            finally
            {
                applyProgressOutput.CompleteProgress();
            }

            return psConfigurationSet;
        }

        private async Task<PSConfigurationSet> GetSetDetailsAsync(PSConfigurationSet psConfigurationSet)
        {
            var processor = psConfigurationSet.PsProcessor.Processor;
            var set = psConfigurationSet.Set;
            var totalUnitsCount = set.ConfigurationUnits.Count;

            if (totalUnitsCount == 0)
            {
                this.Write(StreamType.Warning, Resources.ConfigurationFileEmpty);
                return psConfigurationSet;
            }

            var detailsProgressOutput = new GetConfigurationSetDetailsProgressOutput(
                this,
                this.GetNewProgressActivityId(),
                Resources.ConfigurationGettingDetails,
                Resources.OperationInProgress,
                Resources.OperationCompleted,
                totalUnitsCount);

            var detailsTask = processor.GetSetDetailsAsync(set, ConfigurationUnitDetailLevel.Catalog);
            detailsTask.Progress = detailsProgressOutput.Progress;

            try
            {
                var result = await detailsTask;
                detailsProgressOutput.HandleUnits(result.UnitResults);
            }
            catch (Exception e)
            {
                this.WriteError(
                    ErrorRecordErrorId.ConfigurationDetailsError,
                    e);
            }
            finally
            {
                detailsProgressOutput.CompleteProgress();
            }

            if (detailsProgressOutput.UnitsShown == 0)
            {
                this.Write(StreamType.Warning, Resources.ConfigurationFailedToGetDetails);
                foreach (var unit in set.ConfigurationUnits)
                {
                    var information = new ConfigurationUnitInformation(unit);
                    this.Write(StreamType.Information, information.GetHeader());
                    this.Write(StreamType.Information, information.GetInformation());
                }
            }
            else
            {
                psConfigurationSet.HasDetails = true;
            }

            return psConfigurationSet;
        }

        private void LogFailedGetConfigurationUnitDetails(ConfigurationUnit unit, ConfigurationUnitResultInformation resultInformation)
        {
            if (resultInformation.ResultCode != null)
            {
                string errorMessage = $"Failed to get unit details for {unit.UnitName} 0x{resultInformation.ResultCode.HResult:X}" +
                    $"{Environment.NewLine}Description: '{resultInformation.Description}'{Environment.NewLine}Details: '{resultInformation.Details}'";
                this.WriteError(
                    ErrorRecordErrorId.ConfigurationDetailsError,
                    errorMessage,
                    resultInformation.ResultCode);
            }
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
