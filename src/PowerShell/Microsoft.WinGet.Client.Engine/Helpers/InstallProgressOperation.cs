// -----------------------------------------------------------------------------
// <copyright file="InstallProgressOperation.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Common.Command;
    using Windows.Foundation;

    /// <summary>
    /// Handler progress for install and update.
    /// </summary>
    internal class InstallProgressOperation : ProgressOperationBase<InstallResult, InstallProgress>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InstallProgressOperation"/> class.
        /// </summary>
        /// <param name="pwshCmdlet">A <see cref="PowerShellCmdlet" /> instance.</param>
        /// <param name="activity">Activity.</param>
        /// <param name="operation">Operation.</param>
        public InstallProgressOperation(PowerShellCmdlet pwshCmdlet, string activity, IAsyncOperationWithProgress<InstallResult, InstallProgress> operation)
            : base(pwshCmdlet, activity, operation)
        {
        }

        /// <inheritdoc/>
        public override void Progress(IAsyncOperationWithProgress<InstallResult, InstallProgress> operation, InstallProgress progress)
        {
            ProgressRecord record = new (this.ActivityId, this.Activity, progress.State.ToString())
            {
                RecordType = ProgressRecordType.Processing,
            };

            if (progress.State == PackageInstallProgressState.Downloading && progress.BytesRequired != 0)
            {
                double downloaded = (double)progress.BytesDownloaded / Constants.OneMB;
                double total = (double)progress.BytesRequired / Constants.OneMB;
                record.StatusDescription = $"{downloaded:0.0} MB / {total:0.0} MB";
                record.PercentComplete = (int)(progress.DownloadProgress * 100);
            }
            else if (progress.State == PackageInstallProgressState.Installing)
            {
                record.PercentComplete = (int)(progress.InstallationProgress * 100);
            }

            this.PwshCmdlet.Write(StreamType.Progress, record);
        }
    }
}
