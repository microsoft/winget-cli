// -----------------------------------------------------------------------------
// <copyright file="DownloadOperationWithProgress.cs" company="Microsoft Corporation">
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
    /// Handler progress for package download.
    /// </summary>
    internal class DownloadOperationWithProgress : OperationWithProgressBase<DownloadResult, PackageDownloadProgress>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DownloadOperationWithProgress"/> class.
        /// </summary>
        /// <param name="pwshCmdlet">A <see cref="PowerShellCmdlet" /> instance.</param>
        /// <param name="activity">Activity.</param>
        public DownloadOperationWithProgress(PowerShellCmdlet pwshCmdlet, string activity)
            : base(pwshCmdlet, activity)
        {
        }

        /// <inheritdoc/>
        public override void Progress(IAsyncOperationWithProgress<DownloadResult, PackageDownloadProgress> operation, PackageDownloadProgress progress)
        {
            ProgressRecord record = new (this.ActivityId, this.Activity, progress.State.ToString())
            {
                RecordType = ProgressRecordType.Processing,
            };
            record.StatusDescription = Resources.DownloadingMessage;
            record.PercentComplete = (int)(progress.DownloadProgress * 100);
            this.PwshCmdlet.Write(StreamType.Progress, record);
        }
    }
}
