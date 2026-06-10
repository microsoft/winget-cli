// -----------------------------------------------------------------------------
// <copyright file="ApplyConfigurationSetProgressOutput.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Helpers
{
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Common.Command;
    using Windows.Foundation;

    /// <summary>
    /// Helper to handle progress callbacks from ApplySetAsync.
    /// </summary>
    internal class ApplyConfigurationSetProgressOutput : ConfigurationSetProgressOutputBase<ApplyConfigurationSetResult, ConfigurationSetChangeData>
    {
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
        public ApplyConfigurationSetProgressOutput(PowerShellCmdlet cmd, int activityId, string activity, string inProgressMessage, string completeMessage, int totalUnitsExpected)
            : base(cmd, activityId, activity, inProgressMessage, completeMessage, totalUnitsExpected)
        {
        }

        /// <inheritdoc/>
        public override void Progress(IAsyncOperationWithProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> operation, ConfigurationSetChangeData data)
        {
            if (this.isFirstProgress)
            {
                this.HandleProgress(operation.GetResults());
            }

            switch (data.Change)
            {
                case ConfigurationSetChangeEventType.UnitStateChanged:
                    this.HandleUnitProgress(data.Unit, data.UnitState);
                    break;
            }
        }

        /// <inheritdoc/>
        public override void HandleProgress(ApplyConfigurationSetResult result)
        {
            if (!this.isFirstProgress)
            {
                this.isFirstProgress = false;
                foreach (var unitResult in result.UnitResults)
                {
                    this.HandleUnitProgress(unitResult.Unit, unitResult.State);
                }
            }
        }

        private void HandleUnitProgress(ConfigurationUnit unit, ConfigurationUnitState state)
        {
            if (this.UnitsCompleted.Contains(unit.InstanceIdentifier))
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
                case ConfigurationUnitState.Skipped:
                    this.CompleteUnit(unit);
                    break;
            }
        }
    }
}
