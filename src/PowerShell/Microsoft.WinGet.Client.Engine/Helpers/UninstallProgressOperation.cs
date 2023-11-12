// -----------------------------------------------------------------------------
// <copyright file="UninstallProgressOperation.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.Resources;
    using Windows.Foundation;

    /// <summary>
    /// Handler progress for uninstall.
    /// </summary>
    internal class UninstallProgressOperation : ProgressOperationBase<UninstallResult, UninstallProgress>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="UninstallProgressOperation"/> class.
        /// </summary>
        /// <param name="pwshCmdlet">A <see cref="PowerShellCmdlet" /> instance.</param>
        /// <param name="activity">Activity.</param>
        /// <param name="operation">Operation.</param>
        public UninstallProgressOperation(PowerShellCmdlet pwshCmdlet, string activity, IAsyncOperationWithProgress<UninstallResult, UninstallProgress> operation)
            : base(pwshCmdlet, activity, operation)
        {
            ProgressRecord record = new (this.ActivityId, this.Activity, Resources.Uninstalling)
            {
                RecordType = ProgressRecordType.Processing,
            };
            record.StatusDescription = Resources.Uninstalling;
            record.PercentComplete = 1;
            this.PwshCmdlet.Write(StreamType.Progress, record);
        }

        /// <inheritdoc/>
        public override void Progress(IAsyncOperationWithProgress<UninstallResult, UninstallProgress> operation, UninstallProgress progress)
        {
            ProgressRecord record = new (this.ActivityId, this.Activity, progress.State.ToString())
            {
                RecordType = ProgressRecordType.Processing,
            };
            record.StatusDescription = Resources.Uninstalling;
            record.PercentComplete = (int)(progress.UninstallationProgress * 100);
            this.PwshCmdlet.Write(StreamType.Progress, record);
        }
    }
}
