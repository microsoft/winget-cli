﻿// -----------------------------------------------------------------------------
// <copyright file="ConfigurationCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Commands
{
    using System;
    using System.IO;
    using System.Linq;
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
    using WinRT;

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
        /// <param name="isApply">If prompt is for apply.</param>
        /// <returns>If accepted.</returns>
        public static bool ConfirmConfigurationProcessing(PSCmdlet psCmdlet, bool hasAccepted, bool isApply)
        {
            bool result = false;
            if (!hasAccepted)
            {
                var prompt = isApply ? Resources.ConfigurationWarningPromptApply : Resources.ConfigurationWarningPromptTest;
                bool yesToAll = false;
                bool noToAll = false;
                result = psCmdlet.ShouldContinue(prompt, Resources.ConfigurationWarning, true, ref yesToAll, ref noToAll);

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
        /// <param name="modulePath">The module path to use.</param>
        /// <param name="executionPolicy">Execution policy.</param>
        /// <param name="canUseTelemetry">If telemetry can be used.</param>
        public void Get(
            string configFile,
            string modulePath,
            ExecutionPolicy executionPolicy,
            bool canUseTelemetry)
        {
            var openParams = new OpenConfigurationParameters(
                this.PsCmdlet, configFile, modulePath, executionPolicy, canUseTelemetry);

            // Start task.
            var runningTask = this.RunOnMTA<PSConfigurationSet>(
                async () =>
                {
                    try
                    {
                        return await this.OpenConfigurationSetAsync(openParams);
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
                    throw new InvalidOperationException();
                }

                var runningTask = this.RunOnMTA<PSConfigurationSet>(
                async () =>
                {
                    try
                    {
                        psConfigurationSet = await this.GetSetDetailsAsync(psConfigurationSet, false);
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
            // if (psConfigurationSet.Set.State == ConfigurationSetState.Completed)
            if (psConfigurationSet.ApplyCompleted)
            {
                this.Write(StreamType.Warning, "Processing this set is completed");
                throw new InvalidOperationException();
            }

            if (!psConfigurationSet.CanProcess())
            {
                throw new InvalidOperationException();
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
            if (psConfigurationJob.ApplyConfigurationTask.IsCompleted)
            {
                // It is safe to print all output.
                psConfigurationJob.StartCommand.ConsumeAndWriteStreams(this);

                this.Write(StreamType.Verbose, "The task was completed before waiting");
                if (psConfigurationJob.ApplyConfigurationTask.IsCompletedSuccessfully)
                {
                    this.Write(StreamType.Verbose, "Completed successfully");
                    this.Write(StreamType.Object, psConfigurationJob.ApplyConfigurationTask.Result);
                    return;
                }
                else if (psConfigurationJob.ApplyConfigurationTask.IsFaulted)
                {
                    this.Write(StreamType.Verbose, "Completed faulted before waiting");

                    // Maybe just write error?
                    throw psConfigurationJob.ApplyConfigurationTask.Exception!;
                }
            }

            this.ContinueHelper(psConfigurationJob);
        }

        /// <summary>
        /// Test configuration.
        /// </summary>
        /// <param name="psConfigurationSet">PSConfigurationSet.</param>
        public void Test(PSConfigurationSet psConfigurationSet)
        {
            psConfigurationSet.PsProcessor.UpdateDiagnosticCmdlet(this);

            if (!psConfigurationSet.CanProcess())
            {
                throw new InvalidOperationException();
            }

            var runningTask = this.RunOnMTA<PSTestConfigurationSetResult>(
                async () =>
                {
                    try
                    {
                        return await this.TestConfigurationAsync(psConfigurationSet);
                    }
                    finally
                    {
                        this.Complete();
                        psConfigurationSet.DoneProcessing();
                    }
                });

            this.Wait(runningTask);
            this.Write(StreamType.Object, runningTask.Result);
        }

        /// <summary>
        /// Validates configuration.
        /// </summary>
        /// <param name="psConfigurationSet">PSConfigurationSet.</param>
        public void Validate(PSConfigurationSet psConfigurationSet)
        {
            psConfigurationSet.PsProcessor.UpdateDiagnosticCmdlet(this);

            if (!psConfigurationSet.CanProcess())
            {
                throw new InvalidOperationException();
            }

            var runningTask = this.RunOnMTA<PSValidateConfigurationSetResult>(
                async () =>
                {
                    try
                    {
                        var setResult = await this.ApplyConfigurationAsync(psConfigurationSet, ApplyConfigurationSetFlags.PerformConsistencyCheckOnly);
                        return new PSValidateConfigurationSetResult(setResult);
                    }
                    finally
                    {
                        this.Complete();
                        psConfigurationSet.DoneProcessing();
                    }
                });

            this.Wait(runningTask);
            this.Write(StreamType.Object, runningTask.Result);
        }

        /// <summary>
        /// Cancels a configuration job.
        /// </summary>
        /// <param name="psConfigurationJob">PSConfiguration job.</param>
        public void Cancel(PSConfigurationJob psConfigurationJob)
        {
            psConfigurationJob.StartCommand.Cancel();
        }

        private void ContinueHelper(PSConfigurationJob psConfigurationJob)
        {
            // Signal the command that it can write to streams and wait for task.
            this.Write(StreamType.Verbose, "Waiting for task to complete");
            psConfigurationJob.StartCommand.Wait(psConfigurationJob.ApplyConfigurationTask, this);
            this.Write(StreamType.Object, psConfigurationJob.ApplyConfigurationTask.Result);
        }

        private IConfigurationSetProcessorFactory CreateFactory(OpenConfigurationParameters openParams)
        {
            var factory = new PowerShellConfigurationSetProcessorFactory();

            var properties = factory.As<IPowerShellConfigurationProcessorFactoryProperties>();
            properties.Policy = openParams.Policy;
            properties.ProcessorType = PowerShellConfigurationProcessorType.Default;
            properties.Location = openParams.Location;
            if (properties.Location == PowerShellConfigurationProcessorLocation.Custom)
            {
                properties.CustomLocation = openParams.CustomLocation;
            }

            return factory;
        }

        private async Task<PSConfigurationSet> OpenConfigurationSetAsync(OpenConfigurationParameters openParams)
        {
            this.Write(StreamType.Verbose, Resources.ConfigurationInitializing);

            var psProcessor = new PSConfigurationProcessor(this.CreateFactory(openParams), this, openParams.CanUseTelemetry);

            this.Write(StreamType.Verbose, Resources.ConfigurationReadingConfigFile);
            var stream = await FileRandomAccessStream.OpenAsync(openParams.ConfigFile, FileAccessMode.Read);

            OpenConfigurationSetResult openResult = await psProcessor.Processor.OpenConfigurationSetAsync(stream);
            if (openResult.ResultCode != null)
            {
                throw new OpenConfigurationSetException(openResult, openParams.ConfigFile);
            }

            var set = openResult.Set;

            // This should match winget's OpenConfigurationSet or OpenConfigurationSetAsync
            // should be modify to take the full path and handle it.
            set.Name = Path.GetFileName(openParams.ConfigFile);
            set.Origin = Path.GetDirectoryName(openParams.ConfigFile);
            set.Path = openParams.ConfigFile;

            return new PSConfigurationSet(psProcessor, set);
        }

        private PSConfigurationJob StartApplyInternal(PSConfigurationSet psConfigurationSet)
        {
            psConfigurationSet.PsProcessor.UpdateDiagnosticCmdlet(this);

            var runningTask = this.RunOnMTA<PSApplyConfigurationSetResult>(
                async () =>
                {
                    try
                    {
                        var setResult = await this.ApplyConfigurationAsync(psConfigurationSet, ApplyConfigurationSetFlags.None);
                        psConfigurationSet.ApplyCompleted = true;
                        return new PSApplyConfigurationSetResult(setResult);
                    }
                    finally
                    {
                        this.Complete();
                        psConfigurationSet.DoneProcessing();
                    }
                });

            return new PSConfigurationJob(runningTask, this);
        }

        private async Task<ApplyConfigurationSetResult> ApplyConfigurationAsync(PSConfigurationSet psConfigurationSet, ApplyConfigurationSetFlags flags)
        {
            if (!psConfigurationSet.HasDetails)
            {
                this.Write(StreamType.Verbose, "Getting details for configuration set");
                await this.GetSetDetailsAsync(psConfigurationSet, true);
            }

            var processor = psConfigurationSet.PsProcessor.Processor;
            var set = psConfigurationSet.Set;

            var applyProgressOutput = new ApplyConfigurationSetProgressOutput(
                this,
                this.GetNewProgressActivityId(),
                Resources.ConfigurationApply,
                Resources.OperationInProgress,
                Resources.OperationCompleted,
                set.Units.Count);

            var applyTask = processor.ApplySetAsync(set, flags);
            applyTask.Progress = applyProgressOutput.Progress;

            try
            {
                var result = await applyTask.AsTask(this.GetCancellationToken());
                applyProgressOutput.HandleProgress(result);
                return result;
            }
            finally
            {
                applyProgressOutput.CompleteProgress();
            }
        }

        private async Task<PSTestConfigurationSetResult> TestConfigurationAsync(PSConfigurationSet psConfigurationSet)
        {
            if (!psConfigurationSet.HasDetails)
            {
                this.Write(StreamType.Verbose, "Getting details for configuration set");
                await this.GetSetDetailsAsync(psConfigurationSet, true);
            }

            var processor = psConfigurationSet.PsProcessor.Processor;
            var set = psConfigurationSet.Set;

            var testProgressOutput = new TestConfigurationSetProgressOutput(
                this,
                this.GetNewProgressActivityId(),
                Resources.ConfigurationAssert,
                Resources.OperationInProgress,
                Resources.OperationCompleted,
                set.Units.Count);

            var testTask = processor.TestSetAsync(set);
            testTask.Progress = testProgressOutput.Progress;

            try
            {
                var result = await testTask.AsTask(this.GetCancellationToken());
                testProgressOutput.HandleProgress(result);

                return new PSTestConfigurationSetResult(result);
            }
            finally
            {
                testProgressOutput.CompleteProgress();
            }
        }

        private async Task<PSConfigurationSet> GetSetDetailsAsync(PSConfigurationSet psConfigurationSet, bool warnOnError)
        {
            var processor = psConfigurationSet.PsProcessor.Processor;
            var set = psConfigurationSet.Set;
            var totalUnitsCount = set.Units.Count;

            if (totalUnitsCount == 0)
            {
                this.Write(StreamType.Warning, Resources.ConfigurationFileEmpty);
                return psConfigurationSet;
            }

            try
            {
                var detailsProgressOutput = new GetConfigurationSetDetailsProgressOutput(
                    this,
                    this.GetNewProgressActivityId(),
                    Resources.ConfigurationGettingDetails,
                    Resources.OperationInProgress,
                    Resources.OperationCompleted,
                    totalUnitsCount);

                var detailsTask = processor.GetSetDetailsAsync(set, ConfigurationUnitDetailFlags.ReadOnly);
                detailsTask.Progress = detailsProgressOutput.Progress;

                try
                {
                    var result = await detailsTask.AsTask(this.GetCancellationToken());
                    detailsProgressOutput.HandleProgress(result);

                    if (result.UnitResults.Where(u => u.ResultInformation.ResultCode != null).Any())
                    {
                        throw new GetDetailsException(result.UnitResults);
                    }

                    if (detailsProgressOutput.UnitsShown == 0)
                    {
                        throw new GetDetailsException();
                    }

                    psConfigurationSet.HasDetails = true;
                }
                finally
                {
                    detailsProgressOutput.CompleteProgress();
                }
            }
            catch (Exception e)
            {
                if (warnOnError)
                {
                    this.Write(StreamType.Warning, e.Message);
                }
                else
                {
                    throw;
                }
            }

            return psConfigurationSet;
        }
    }
}
