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

        private void HandleUnitProgress(ConfigurationUnit unit, ConfigurationUnitState state, IConfigurationUnitResultInformation resultInformation)
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
                    break;
                case ConfigurationUnitState.Completed:
                    if (resultInformation.ResultCode != null)
                    {
                        string description = resultInformation.Description.Trim();
                        var message = this.GetUnitFailedMessage(unit, resultInformation);

                        string errorMessage = $"Configuration unit {unit.UnitName}[{unit.Identifier}] failed with code 0x{resultInformation.ResultCode.HResult:X}" +
                            $" and error message:\n{description}\n{resultInformation.Details}\n{message}";
                        this.cmd.WriteError(
                            ErrorRecordErrorId.ConfigurationApplyError,
                            errorMessage,
                            resultInformation.ResultCode);
                    }

                    this.CompleteUnit(unit);
                    break;
                case ConfigurationUnitState.Skipped:
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

        private string GetUnitFailedMessage(ConfigurationUnit unit, IConfigurationUnitResultInformation resultInformation)
        {
            if (resultInformation.ResultCode == null)
            {
                return string.Format(Resources.ConfigurationUnitFailed, "null");
            }

            int resultCode = resultInformation.ResultCode.HResult;
            switch (resultCode)
            {
                case ErrorCodes.WingetConfigErrorDuplicateIdentifier:
                    return string.Format(Resources.ConfigurationUnitHasDuplicateIdentifier, unit.Identifier);
                case ErrorCodes.WingetConfigErrorMissingDependency:
                    return string.Format(Resources.ConfigurationUnitHasMissingDependency, resultInformation.Details);
                case ErrorCodes.WingetConfigErrorAssertionFailed:
                    return Resources.ConfigurationUnitAssertHadNegativeResult;
                case ErrorCodes.WinGetConfigUnitNotFound:
                    return Resources.ConfigurationUnitNotFoundInModule;
                case ErrorCodes.WinGetConfigUnitNotFoundRepository:
                    return Resources.ConfigurationUnitNotFound;
                case ErrorCodes.WinGetConfigUnitMultipleMatches:
                    return Resources.ConfigurationUnitMultipleMatches;
                case ErrorCodes.WinGetConfigUnitInvokeGet:
                    return Resources.ConfigurationUnitFailedDuringGet;
                case ErrorCodes.WinGetConfigUnitInvokeTest:
                    return Resources.ConfigurationUnitFailedDuringTest;
                case ErrorCodes.WinGetConfigUnitInvokeSet:
                    return Resources.ConfigurationUnitFailedDuringSet;
                case ErrorCodes.WinGetConfigUnitModuleConflict:
                    return Resources.ConfigurationUnitModuleConflict;
                case ErrorCodes.WinGetConfigUnitImportModule:
                    return Resources.ConfigurationUnitModuleImportFailed;
                case ErrorCodes.WinGetConfigUnitInvokeInvalidResult:
                    return Resources.ConfigurationUnitReturnedInvalidResult;
            }

            switch (resultInformation.ResultSource)
            {
                case ConfigurationUnitResultSource.ConfigurationSet:
                    return string.Format(Resources.ConfigurationUnitFailedConfigSet, resultCode);
                case ConfigurationUnitResultSource.Internal:
                    return string.Format(Resources.ConfigurationUnitFailedInternal, resultCode);
                case ConfigurationUnitResultSource.Precondition:
                    return string.Format(Resources.ConfigurationUnitFailedPrecondition, resultCode);
                case ConfigurationUnitResultSource.SystemState:
                    return string.Format(Resources.ConfigurationUnitFailedSystemState, resultCode);
                case ConfigurationUnitResultSource.UnitProcessing:
                    return string.Format(Resources.ConfigurationUnitFailedUnitProcessing, resultCode);
            }

            return string.Format(Resources.ConfigurationUnitFailed, resultCode);
        }

        private string GetUnitSkippedMessage(IConfigurationUnitResultInformation resultInformation)
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
