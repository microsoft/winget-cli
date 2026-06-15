// -----------------------------------------------------------------------------
// <copyright file="OperationWithProgressBase.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.Runtime.InteropServices;
    using System.Threading.Tasks;
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.Resources;
    using Windows.Foundation;

    /// <summary>
    /// Base class for async operations with progress.
    /// </summary>
    /// <typeparam name="TOperationResult">The operation result.</typeparam>
    /// <typeparam name="TProgressData">Progress data.</typeparam>
    internal abstract class OperationWithProgressBase<TOperationResult, TProgressData>
    {
        private static bool isProgressEnabled;

        static OperationWithProgressBase()
        {
            // Progress on arm64 will produce an AV because there's an OS bug where marshaling structs over a certain size fail.
            // Fix is in 10.0.26068.0, for build before that disable progress.
            if (RuntimeInformation.ProcessArchitecture == Architecture.Arm64)
            {
                var minWindowsVersion = new Version(10, 0, 26068, 0);
                var osVersion = Environment.OSVersion.Version;
                isProgressEnabled = osVersion.CompareTo(minWindowsVersion) >= 0;
            }
            else
            {
                isProgressEnabled = true;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="OperationWithProgressBase{TOperationResult, TProgressData}"/> class.
        /// </summary>
        /// <param name="pwshCmdlet">A <see cref="PowerShellCmdlet" /> instance.</param>
        /// <param name="activity">Activity.</param>
        public OperationWithProgressBase(
            PowerShellCmdlet pwshCmdlet,
            string activity)
        {
            this.PwshCmdlet = pwshCmdlet;
            this.ActivityId = pwshCmdlet.GetNewProgressActivityId();
            this.Activity = activity;
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
        /// Starts the operation and executes it as task.
        /// Supports cancellation.
        /// </summary>
        /// <param name="func">Lambda with operation.</param>
        /// <returns>TOperationReturn.</returns>
        public async Task<TOperationResult> ExecuteAsync(Func<IAsyncOperationWithProgress<TOperationResult, TProgressData>> func)
        {
            var operation = func();

            if (isProgressEnabled)
            {
                operation.Progress = this.Progress;
            }

            try
            {
                return await operation.AsTask(this.PwshCmdlet.GetCancellationToken());
            }
            finally
            {
                this.Complete();
            }
        }

        /// <summary>
        /// Completes progress for this activity.
        /// </summary>
        protected virtual void Complete()
        {
            this.PwshCmdlet.CompleteProgress(this.ActivityId, this.Activity, Resources.Completed, true);
        }
    }
}
