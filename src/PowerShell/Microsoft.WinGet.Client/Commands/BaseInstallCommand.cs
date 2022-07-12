// -----------------------------------------------------------------------------
// <copyright file="BaseInstallCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

#pragma warning disable SA1200 // Using directives should be placed correctly
using Windows.Foundation;
#pragma warning restore SA1200 // Using directives should be placed correctly

namespace Microsoft.WinGet.Client.Commands
{
    using System.IO;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// This is the base class for the install and upgrade commands.
    /// </summary>
    public class BaseInstallCommand : BaseLifecycleCommand
    {
        private string location;

        /// <summary>
        /// Gets or sets the mode to manipulate the package with.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PackageInstallMode Mode { get; set; } = PackageInstallMode.Default;

        /// <summary>
        /// Gets or sets the override arguments to be passed on to the installer.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Override { get; set; }

        /// <summary>
        /// Gets or sets the installation location.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Location
        {
            get => this.location;
            set
            {
                string prefix = Path.IsPathRooted(value)
                    ? string.Empty
                    : this.SessionState.Path.CurrentFileSystemLocation + @"\";

                this.location = prefix + value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to skip the installer hash validation check.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Force { get; set; }

        /// <summary>
        /// Gets or sets the optional HTTP Header to pass on to the REST Source.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Header { get; set; }

        /// <summary>
        /// Gets the install options from the configured parameters.
        /// </summary>
        /// <param name="version">The <see cref="PackageVersionId" /> to install.</param>
        /// <returns>An <see cref="InstallOptions" /> instance.</returns>
        protected virtual InstallOptions GetInstallOptions(PackageVersionId version)
        {
            var options = ComObjectFactory.Value.CreateInstallOptions();

            options.AllowHashMismatch = this.Force.ToBool();
            options.PackageInstallMode = this.Mode;

            if (version != null)
            {
                options.PackageVersionId = version;
            }

            if (this.Log != null)
            {
                options.LogOutputPath = this.Log;
            }

            if (this.Override != null)
            {
                options.ReplacementInstallerArguments = this.Override;
            }

            if (this.Location != null)
            {
                options.PreferredInstallLocation = this.Location;
            }

            if (this.Header != null)
            {
                options.AdditionalPackageCatalogArguments = this.Header;
            }

            return options;
        }

        /// <summary>
        /// Registers callbacks on an asynchronous operation and waits for the results.
        /// </summary>
        /// <param name="operation">The asynchronous operation.</param>
        /// <param name="activity">A <see cref="string" /> instance.</param>
        /// <returns>A <see cref="InstallResult" /> instance.</returns>
        protected InstallResult RegisterCallbacksAndWait(
            IAsyncOperationWithProgress<InstallResult, InstallProgress> operation,
            string activity)
        {
            var adapter = new Helpers.WriteProgressAdapter(this);

            operation.Progress = (context, progress) =>
            {
                var record = new ProgressRecord(1, activity, progress.State.ToString())
                {
                    RecordType = ProgressRecordType.Processing,
                };

                if ((progress.State == PackageInstallProgressState.Downloading) && (progress.BytesRequired != 0))
                {
                    record.StatusDescription = $"{progress.BytesDownloaded / 1000000.0f:0.0} MB / {progress.BytesRequired / 1000000.0f:0.0} MB";
                    record.PercentComplete = (int)(progress.DownloadProgress * 100);
                }
                else if (progress.State == PackageInstallProgressState.Installing)
                {
                    record.PercentComplete = (int)(progress.InstallationProgress * 100);
                }

                adapter.WriteProgress(record);
            };

            operation.Completed = (context, status) =>
            {
                var record = new ProgressRecord(1, activity, status.ToString())
                {
                    RecordType = ProgressRecordType.Completed,
                };

                adapter.WriteProgress(record);
                adapter.Completed = true;
            };

            System.Console.CancelKeyPress += (sender, e) =>
            {
                operation.Cancel();
            };

            adapter.Wait();
            return operation.GetResults();
        }
    }
}
