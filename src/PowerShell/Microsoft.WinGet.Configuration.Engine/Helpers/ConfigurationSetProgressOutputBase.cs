// -----------------------------------------------------------------------------
// <copyright file="ConfigurationSetProgressOutputBase.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Helpers
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Common.Command;
    using Windows.Foundation;

    /// <summary>
    /// Helper to handle progress callbacks.
    /// </summary>
    /// <typeparam name="TOperationResult">The operation result.</typeparam>
    /// <typeparam name="TProgressData">Progress data.</typeparam>
    internal abstract class ConfigurationSetProgressOutputBase<TOperationResult, TProgressData>
    {
        private readonly PowerShellCmdlet cmd;
        private readonly int activityId;
        private readonly string activity;
        private readonly string inProgressMessage;
        private readonly string completeMessage;
        private readonly int totalUnitsExpected;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationSetProgressOutputBase{TOperationResult, TProgressData}"/> class.
        /// </summary>
        /// <param name="cmd">Command that outputs the messages.</param>
        /// <param name="activityId">The activity id of the progress bar.</param>
        /// <param name="activity">The activity.</param>
        /// <param name="inProgressMessage">The message in the progress bar.</param>
        /// <param name="completeMessage">The activity complete message.</param>
        /// <param name="totalUnitsExpected">Total of units expected.</param>
        public ConfigurationSetProgressOutputBase(PowerShellCmdlet cmd, int activityId, string activity, string inProgressMessage, string completeMessage, int totalUnitsExpected)
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
        /// Gets or sets a hash set with the completed units.
        /// </summary>
        protected HashSet<Guid> UnitsCompleted { get; set; } = new ();

        /// <summary>
        /// Progress callback.
        /// </summary>
        /// <param name="operation">Async operation in progress.</param>
        /// <param name="data">Change data.</param>
        public abstract void Progress(IAsyncOperationWithProgress<TOperationResult, TProgressData> operation, TProgressData data);

        /// <summary>
        /// Handle progress.
        /// </summary>
        /// <param name="result">Set result.</param>
        public abstract void HandleProgress(TOperationResult result);

        /// <summary>
        /// Completes the progress bar.
        /// </summary>
        public void CompleteProgress()
        {
            this.cmd.CompleteProgress(this.activityId, this.activity, this.completeMessage);
        }

        /// <summary>
        /// Marks a unit as completed and increase progress.
        /// </summary>
        /// <param name="unit">Unit.</param>
        protected void CompleteUnit(ConfigurationUnit unit)
        {
            if (this.UnitsCompleted.Add(unit.InstanceIdentifier))
            {
                this.cmd.WriteProgressWithPercentage(this.activityId, this.activity, $"{this.inProgressMessage} {this.UnitsCompleted.Count}/{this.totalUnitsExpected}", this.UnitsCompleted.Count, this.totalUnitsExpected);
            }
        }
    }
}
