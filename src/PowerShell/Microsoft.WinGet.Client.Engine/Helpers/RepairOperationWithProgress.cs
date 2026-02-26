// -----------------------------------------------------------------------------
// <copyright file="RepairOperationWithProgress.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.Resources;
    using Windows.Foundation;

    /// <summary>
    /// Handler for Repair Operation with Progress.
    /// </summary>
    internal class RepairOperationWithProgress : OperationWithProgressBase<RepairResult, RepairProgress>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="RepairOperationWithProgress"/> class.
        /// </summary>
        /// <param name="pwshCmdlet"> instance.</param>
        /// <param name="activity">Activity.</param>
        public RepairOperationWithProgress(PowerShellCmdlet pwshCmdlet, string activity)
            : base(pwshCmdlet, activity)
        {
        }

        /// <inheritdoc/>
        public override void Progress(IAsyncOperationWithProgress<RepairResult, RepairProgress> operation, RepairProgress progress)
        {
            ProgressRecord record = new (this.ActivityId, this.Activity, progress.State.ToString())
            {
                RecordType = ProgressRecordType.Processing,
            };

            record.StatusDescription = Resources.Repairing;
            record.PercentComplete = (int)(progress.RepairCompletionProgress * 100);
            this.PwshCmdlet.Write(StreamType.Progress, record);
        }
    }
}
