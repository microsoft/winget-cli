// -----------------------------------------------------------------------------
// <copyright file="GetConfigurationSetDetailsProgressOutput.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Helpers
{
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Common.Command;
    using Windows.Foundation;

    /// <summary>
    /// Helper to handle progress callback from GetSetDetailsAsync.
    /// </summary>
    internal class GetConfigurationSetDetailsProgressOutput : ConfigurationSetProgressOutputBase<GetConfigurationSetDetailsResult, GetConfigurationUnitDetailsResult>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GetConfigurationSetDetailsProgressOutput"/> class.
        /// </summary>
        /// <param name="cmd">Command that outputs the messages.</param>
        /// <param name="activityId">The activity id of the progress bar.</param>
        /// <param name="activity">The activity.</param>
        /// <param name="inProgressMessage">The message in the progress bar.</param>
        /// <param name="completeMessage">The activity complete message.</param>
        /// <param name="totalUnitsExpected">Total of units expected.</param>
        public GetConfigurationSetDetailsProgressOutput(PowerShellCmdlet cmd, int activityId, string activity, string inProgressMessage, string completeMessage, int totalUnitsExpected)
            : base(cmd, activityId, activity, inProgressMessage, completeMessage, totalUnitsExpected)
        {
        }

        /// <summary>
        /// Gets the units shown.
        /// </summary>
        public int UnitsShown
        {
            get { return this.UnitsCompleted.Count; }
        }

        /// <inheritdoc/>
        public override void Progress(IAsyncOperationWithProgress<GetConfigurationSetDetailsResult, GetConfigurationUnitDetailsResult> operation, GetConfigurationUnitDetailsResult result)
        {
            this.HandleProgress(operation.GetResults());
        }

        /// <inheritdoc/>
        public override void HandleProgress(GetConfigurationSetDetailsResult result)
        {
            foreach (var unitResult in result.UnitResults)
            {
                this.CompleteUnit(unitResult.Unit);
            }
        }
    }
}
