// -----------------------------------------------------------------------------
// <copyright file="ProgressOperationBase.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.Threading.Tasks;
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.Resources;
    using Windows.Foundation;

    /// <summary>
    /// Base class to handle progress.
    /// </summary>
    /// <typeparam name="TOperationResult">The operation result.</typeparam>
    /// <typeparam name="TProgressData">Progress data.</typeparam>
    internal abstract class ProgressOperationBase<TOperationResult, TProgressData>
    {
        private IAsyncOperationWithProgress<TOperationResult, TProgressData> operation;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProgressOperationBase{TOperationResult, TProgressData}"/> class.
        /// </summary>
        /// <param name="pwshCmdlet">A <see cref="PowerShellCmdlet" /> instance.</param>
        /// <param name="activity">Activity.</param>
        /// <param name="operation">Operation.</param>
        public ProgressOperationBase(
            PowerShellCmdlet pwshCmdlet,
            string activity,
            IAsyncOperationWithProgress<TOperationResult, TProgressData> operation)
        {
            this.PwshCmdlet = pwshCmdlet;
            this.ActivityId = pwshCmdlet.GetNewProgressActivityId();
            this.Activity = activity;
            this.operation = operation;

            operation.Progress = this.Progress;
        }

        /// <summary>
        /// Gets the PowerShellCmdlet.
        /// </summary>
        protected PowerShellCmdlet PwshCmdlet { get; }

        /// <summary>
        /// Gets the progress activity id.
        /// </summary>
        protected int ActivityId { get; }

        /// <summary>
        /// Gets the activity.
        /// </summary>
        protected string Activity { get; }

        /// <summary>
        /// Progress callback.
        /// </summary>
        /// <param name="operation">Async operation in progress.</param>
        /// <param name="progress">Progress data.</param>
        public abstract void Progress(IAsyncOperationWithProgress<TOperationResult, TProgressData> operation, TProgressData progress);

        /// <summary>
        /// Gets the result of the async operation.
        /// Supports cancellation.
        /// </summary>
        /// <returns>TOperationReturn.</returns>
        public async Task<TOperationResult> GetResult()
        {
            return await this.operation.AsTask(this.PwshCmdlet.GetCancellationToken());
        }

        /// <summary>
        /// Completes progress for this activity.
        /// </summary>
        public void CompleteProgress()
        {
            this.PwshCmdlet.CompleteProgress(this.ActivityId, this.Activity, Resources.Completed, true);
        }
    }
}
