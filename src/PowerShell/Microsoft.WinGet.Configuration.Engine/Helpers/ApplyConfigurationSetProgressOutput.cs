// -----------------------------------------------------------------------------
// <copyright file="ApplyConfigurationSetProgressOutput.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Engine.Exceptions;
    using Microsoft.WinGet.Configuration.Engine.Resources;
    using Windows.Foundation;
    using static Microsoft.WinGet.Configuration.Engine.Commands.AsyncCommand;

    /// <summary>
    /// Helper to handle progress callbacks from ApplyConfigurationSetAsync.
    /// </summary>
    internal class ApplyConfigurationSetProgressOutput
    {
        private readonly AsyncCommand cmd;
        private readonly int activityId;
        private readonly string activity;
        private readonly string inProgressMessage;
        private readonly string completeMessage;
        private readonly int totalUnitsExpected;

        private readonly HashSet<Guid> unitsSeen = new ();
        private readonly HashSet<Guid> unitsCompleted = new ();

        private bool isFirstProgress = true;

        /// <summary>
        /// Initializes a new instance of the <see cref="ApplyConfigurationSetProgressOutput"/> class.
        /// </summary>
        /// <param name="cmd">Command that outputs the messages.</param>
        /// <param name="activityId">The activity id of the progress bar.</param>
        /// <param name="activity">The activity.</param>
        /// <param name="inProgressMessage">The message in the progress bar.</param>
        /// <param name="completeMessage">The activity complete message.</param>
        /// <param name="totalUnitsExpected">Total of units expected.</param>
        public ApplyConfigurationSetProgressOutput(AsyncCommand cmd, int activityId, string activity, string inProgressMessage, string completeMessage, int totalUnitsExpected)
        {
            this.cmd = cmd;
            this.activityId = activityId;
            this.activity = activity;
            this.inProgressMessage = inProgressMessage;
            this.completeMessage = completeMessage;
            this.totalUnitsExpected = totalUnitsExpected;

            // Write initial progress record.
            // For some reason, if this is 0 the progress bar is shown full. Start with 1%
            this.cmd.WriteProgressWithPercentage(activityId, activity, $"{this.inProgressMessage} 0/{this.totalUnitsExpected}", 1, 100);
        }

        /// <summary>
        /// Progress callback.
        /// </summary>
        /// <param name="operation">Async operation in progress.</param>
        /// <param name="data">Change data.</param>
        public void Progress(IAsyncOperationWithProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> operation, ConfigurationSetChangeData data)
        {
            if (this.isFirstProgress)
            {
                this.HandleUnreportedProgress(operation.GetResults());
            }

            switch (data.Change)
            {
                case ConfigurationSetChangeEventType.SetStateChanged:
                    switch (data.SetState)
                    {
                        case ConfigurationSetState.Pending:
                            this.cmd.Write(StreamType.Information, Utilities.CreateInformationMessage(Resources.ConfigurationWaitingOnAnother));
                            break;
                    }

                    break;
                case ConfigurationSetChangeEventType.UnitStateChanged:
                    this.HandleUnitProgress(data.Unit, data.UnitState, data.ResultInformation);
                    break;
            }
        }

        /// <summary>
        /// Handle unreported progress.
        /// </summary>
        /// <param name="result">Set result.</param>
        public void HandleUnreportedProgress(ApplyConfigurationSetResult result)
        {
            if (!this.isFirstProgress)
            {
                this.isFirstProgress = false;
                foreach (var unitResult in result.UnitResults)
                {
                    this.HandleUnitProgress(unitResult.Unit, unitResult.State, unitResult.ResultInformation);
                }
            }
        }

        /// <summary>
        /// Completes the progress bar.
        /// </summary>
        public void CompleteProgress()
        {
            this.cmd.CompleteProgress(this.activityId, this.activity, this.completeMessage);
        }

        private void HandleUnitProgress(ConfigurationUnit unit, ConfigurationUnitState state, ConfigurationUnitResultInformation resultInformation)
        {
            if (this.unitsCompleted.Contains(unit.InstanceIdentifier))
            {
                return;
            }

            switch (state)
            {
                case ConfigurationUnitState.Pending:
                    // The unreported progress handler may send pending units, just ignore them
                    break;
                case ConfigurationUnitState.InProgress:
                    this.OutputUnitInProgressIfNeeded(unit);
                    break;
                case ConfigurationUnitState.Completed:
                    this.OutputUnitInProgressIfNeeded(unit);
                    if (resultInformation.ResultCode == null)
                    {
                        this.cmd.Write(StreamType.Information, Utilities.CreateInformationMessage($"  {Resources.ConfigurationSuccessfullyApplied}"));
                    }
                    else
                    {
                        string description = resultInformation.Description.Trim();
                        var (message, showDescription) = this.GetUnitFailedMessage(unit, resultInformation);
                        var sb = new StringBuilder();
                        sb.AppendLine($"  {message}");

                        if (showDescription && !string.IsNullOrEmpty(description))
                        {
                            bool wasLimited = false;
                            const int maxLines = 3;
                            var lines = Utilities.SplitIntoLines(description, maxLines + 1);

                            for (int i = 0; i < lines.Length && i < maxLines; i++)
                            {
                                sb.AppendLine(lines[i]);
                            }

                            if (lines.Length > maxLines)
                            {
                                wasLimited = true;
                            }

                            if (wasLimited || string.IsNullOrEmpty(resultInformation.Details))
                            {
                                sb.AppendLine(Resources.ConfigurationDescriptionWasTruncated);
                            }
                        }

                        this.cmd.Write(StreamType.Information, Utilities.CreateInformationMessage(sb.ToString(), foregroundColor: ConsoleColor.DarkRed));

                        string errorMessage = $"Configuration unit {unit.UnitName}[{unit.Identifier}] failed with code 0x{resultInformation.ResultCode.HResult:X}" +
                            $" and error message:\n{description}\n{resultInformation.Details}";
                        this.cmd.WriteError(
                            ErrorRecordErrorId.ConfigurationApplyError,
                            errorMessage,
                            resultInformation.ResultCode);
                    }

                    this.CompleteUnit(unit);
                    break;
                case ConfigurationUnitState.Skipped:
                    this.OutputUnitInProgressIfNeeded(unit);
                    this.cmd.Write(StreamType.Warning, this.GetUnitSkippedMessage(resultInformation));
                    this.CompleteUnit(unit);
                    break;
            }
        }

        private void CompleteUnit(ConfigurationUnit unit)
        {
            if (this.unitsCompleted.Add(unit.InstanceIdentifier))
            {
                this.cmd.WriteProgressWithPercentage(this.activityId, this.activity, $"{this.inProgressMessage} {this.unitsCompleted.Count}/{this.totalUnitsExpected}", this.unitsCompleted.Count, this.totalUnitsExpected);
            }
        }

        private void OutputUnitInProgressIfNeeded(ConfigurationUnit unit)
        {
            var unitInstace = unit.InstanceIdentifier;
            if (!this.unitsSeen.Contains(unitInstace))
            {
                this.unitsSeen.Add(unitInstace);
                var unitInfo = new ConfigurationUnitInformation(unit);
                this.cmd.Write(StreamType.Information, unitInfo.GetHeader());
            }
        }

        private (string message, bool showDescription) GetUnitFailedMessage(ConfigurationUnit unit, ConfigurationUnitResultInformation resultInformation)
        {
            if (resultInformation.ResultCode == null)
            {
                return (string.Format(Resources.ConfigurationUnitFailed, "null"), false);
            }

            int resultCode = resultInformation.ResultCode.HResult;
            switch (resultCode)
            {
                case ErrorCodes.WingetConfigErrorDuplicateIdentifier:
                    return (string.Format(Resources.ConfigurationUnitHasDuplicateIdentifier, unit.Identifier), false);
                case ErrorCodes.WingetConfigErrorMissingDependency:
                    return (string.Format(Resources.ConfigurationUnitHasMissingDependency, resultInformation.Details), false);
                case ErrorCodes.WingetConfigErrorAssertionFailed:
                    return (Resources.ConfigurationUnitAssertHadNegativeResult, false);
                case ErrorCodes.WinGetConfigUnitNotFound:
                    return (Resources.ConfigurationUnitNotFoundInModule, false);
                case ErrorCodes.WinGetConfigUnitNotFoundRepository:
                    return (Resources.ConfigurationUnitNotFound, false);
                case ErrorCodes.WinGetConfigUnitMultipleMatches:
                    return (Resources.ConfigurationUnitMultipleMatches, false);
                case ErrorCodes.WinGetConfigUnitInvokeGet:
                    return (Resources.ConfigurationUnitFailedDuringGet, true);
                case ErrorCodes.WinGetConfigUnitInvokeTest:
                    return (Resources.ConfigurationUnitFailedDuringTest, true);
                case ErrorCodes.WinGetConfigUnitInvokeSet:
                    return (Resources.ConfigurationUnitFailedDuringSet, true);
                case ErrorCodes.WinGetConfigUnitModuleConflict:
                    return (Resources.ConfigurationUnitModuleConflict, false);
                case ErrorCodes.WinGetConfigUnitImportModule:
                    return (Resources.ConfigurationUnitModuleImportFailed, true);
                case ErrorCodes.WinGetConfigUnitInvokeInvalidResult:
                    return (Resources.ConfigurationUnitReturnedInvalidResult, false);
            }

            switch (resultInformation.ResultSource)
            {
                case ConfigurationUnitResultSource.ConfigurationSet:
                    return (string.Format(Resources.ConfigurationUnitFailedConfigSet, resultCode), true);
                case ConfigurationUnitResultSource.Internal:
                    return (string.Format(Resources.ConfigurationUnitFailedInternal, resultCode), true);
                case ConfigurationUnitResultSource.Precondition:
                    return (string.Format(Resources.ConfigurationUnitFailedPrecondition, resultCode), true);
                case ConfigurationUnitResultSource.SystemState:
                    return (string.Format(Resources.ConfigurationUnitFailedSystemState, resultCode), true);
                case ConfigurationUnitResultSource.UnitProcessing:
                    return (string.Format(Resources.ConfigurationUnitFailedUnitProcessing, resultCode), true);
            }

            return (string.Format(Resources.ConfigurationUnitFailed, resultCode), true);
        }

        private string GetUnitSkippedMessage(ConfigurationUnitResultInformation resultInformation)
        {
            if (resultInformation.ResultCode == null)
            {
                return string.Format(Resources.ConfigurationUnitSkipped, "null");
            }

            int resultCode = resultInformation.ResultCode.HResult;
            switch (resultCode)
            {
                case ErrorCodes.WingetConfigErrorManuallySkipped:
                    return Resources.ConfigurationUnitManuallySkipped;
                case ErrorCodes.WingetConfigErrorDependencyUnsatisfied:
                    return Resources.ConfigurationUnitNotRunDueToDependency;
                case ErrorCodes.WingetConfigErrorAssertionFailed:
                    return Resources.ConfigurationUnitNotRunDueToFailedAssert;
            }

            return string.Format(Resources.ConfigurationUnitSkipped, resultCode);
        }
    }
}
