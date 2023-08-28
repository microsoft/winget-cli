// -----------------------------------------------------------------------------
// <copyright file="GetConfigurationSetDetailsProgressOutput.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Helpers
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Engine.Exceptions;
    using Windows.Foundation;
    using static Microsoft.WinGet.Configuration.Engine.Commands.AsyncCommand;

    /// <summary>
    /// Helper to handle progress callback from GetSetDetailsAsync.
    /// </summary>
    internal class GetConfigurationSetDetailsProgressOutput
    {
        private readonly AsyncCommand cmd;
        private readonly int activityId;
        private readonly string activity;
        private readonly string inProgressMessage;
        private readonly string completeMessage;
        private readonly int totalUnitsExpected;

        /// <summary>
        /// Initializes a new instance of the <see cref="GetConfigurationSetDetailsProgressOutput"/> class.
        /// </summary>
        /// <param name="cmd">Command that outputs the messages.</param>
        /// <param name="activityId">The activity id of the progress bar.</param>
        /// <param name="activity">The activity.</param>
        /// <param name="inProgressMessage">The message in the progress bar.</param>
        /// <param name="completeMessage">The activity complete message.</param>
        /// <param name="totalUnitsExpected">Total of units expected.</param>
        public GetConfigurationSetDetailsProgressOutput(AsyncCommand cmd, int activityId, string activity, string inProgressMessage, string completeMessage, int totalUnitsExpected)
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
        /// Gets the number of units shown.
        /// </summary>
        internal int UnitsShown { get; private set; } = 0;

        /// <summary>
        /// Progress callback.
        /// </summary>
        /// <param name="operation">Async operation in progress.</param>
        /// <param name="result">Result.</param>
        public void Progress(IAsyncOperationWithProgress<GetConfigurationSetDetailsResult, GetConfigurationUnitDetailsResult> operation, GetConfigurationUnitDetailsResult result)
        {
            this.HandleUnits(operation.GetResults().UnitResults);
        }

        /// <summary>
        /// Handle units.
        /// </summary>
        /// <param name="unitResults">The unit results.</param>
        public void HandleUnits(IReadOnlyList<GetConfigurationUnitDetailsResult> unitResults)
        {
            while (this.UnitsShown < unitResults.Count)
            {
                GetConfigurationUnitDetailsResult unitResult = unitResults[this.UnitsShown];
                this.LogFailedGetConfigurationUnitDetails(unitResult.Unit, unitResult.ResultInformation);
                ++this.UnitsShown;
                this.cmd.WriteProgressWithPercentage(this.activityId, this.activity, $"{this.inProgressMessage} {this.UnitsShown}/{this.totalUnitsExpected}", this.UnitsShown, this.totalUnitsExpected);
            }
        }

        /// <summary>
        /// Complete progress.
        /// </summary>
        public void CompleteProgress()
        {
            this.cmd.CompleteProgress(this.activityId, this.activity, this.completeMessage);
        }

        private void LogFailedGetConfigurationUnitDetails(ConfigurationUnit unit, IConfigurationUnitResultInformation resultInformation)
        {
            if (resultInformation.ResultCode != null)
            {
                string errorMessage = $"Failed to get unit details for {unit.UnitName} 0x{resultInformation.ResultCode.HResult:X}" +
                    $"{Environment.NewLine}Description: '{resultInformation.Description}'{Environment.NewLine}Details: '{resultInformation.Details}'";
                this.cmd.WriteError(
                    ErrorRecordErrorId.ConfigurationDetailsError,
                    errorMessage,
                    resultInformation.ResultCode);
            }
        }
    }
}
