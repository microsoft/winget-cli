// -----------------------------------------------------------------------------
// <copyright file="TestConfigurationSetProgressOutput.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Helpers
{
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Common.Command;
    using Windows.Foundation;

    /// <summary>
    /// Helper to handle progress callbacks for TestSetAsync.
    /// </summary>
    internal class TestConfigurationSetProgressOutput : ConfigurationSetProgressOutputBase<TestConfigurationSetResult, TestConfigurationUnitResult>
    {
        private bool isFirstProgress = true;

        /// <summary>
        /// Initializes a new instance of the <see cref="TestConfigurationSetProgressOutput"/> class.
        /// </summary>
        /// <param name="cmd">Command that outputs the messages.</param>
        /// <param name="activityId">The activity id of the progress bar.</param>
        /// <param name="activity">The activity.</param>
        /// <param name="inProgressMessage">The message in the progress bar.</param>
        /// <param name="completeMessage">The activity complete message.</param>
        /// <param name="totalUnitsExpected">Total of units expected.</param>
        public TestConfigurationSetProgressOutput(PowerShellCmdlet cmd, int activityId, string activity, string inProgressMessage, string completeMessage, int totalUnitsExpected)
            : base(cmd, activityId, activity, inProgressMessage, completeMessage, totalUnitsExpected)
        {
        }

        /// <inheritdoc/>
        public override void Progress(IAsyncOperationWithProgress<TestConfigurationSetResult, TestConfigurationUnitResult> operation, TestConfigurationUnitResult data)
        {
            if (this.isFirstProgress)
            {
                this.HandleProgress(operation.GetResults());
            }

            this.CompleteUnit(data.Unit);
        }

        /// <inheritdoc/>
        public override void HandleProgress(TestConfigurationSetResult result)
        {
            if (!this.isFirstProgress)
            {
                this.isFirstProgress = false;
                foreach (var unitResult in result.UnitResults)
                {
                    this.CompleteUnit(unitResult.Unit);
                }
            }
        }
    }
}
