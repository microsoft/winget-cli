// -----------------------------------------------------------------------------
// <copyright file="BaseInstallCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands.Common
{
    using System.IO;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Helpers;
    using Windows.Foundation;

    /// <summary>
    /// This is the base class for all commands that parse a <see cref="FindPackagesOptions" /> result
    /// from the provided parameters i.e., the "install" and "upgrade" commands.
    /// </summary>
    public abstract class BaseInstallCommand : BasePackageCommand
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
        /// Gets or sets the arguments to be passed on to the installer in addition to the defaults.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Custom { get; set; }

        /// <summary>
        /// Gets or sets the installation location.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Location
        {
            get => this.location;
            set
            {
                this.location = Path.IsPathRooted(value)
                    ? value
                    : this.SessionState.Path.CurrentFileSystemLocation + @"\" + value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether to skip the installer hash validation check.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter AllowHashMismatch { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to continue upon non security related failures.
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
            InstallOptions options = ComObjectFactory.Value.CreateInstallOptions();
            options.AllowHashMismatch = this.AllowHashMismatch.ToBool();
            options.Force = this.Force.ToBool();
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

            // Since these arguments are appended to the installer at runtime, it doesn't make sense to append them if they are whitespace
            if (!string.IsNullOrWhiteSpace(this.Custom))
            {
                options.AdditionalInstallerArguments = this.Custom;
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
            WriteProgressAdapter adapter = new (this);
            operation.Progress = (context, progress) =>
            {
                ProgressRecord record = new (1, activity, progress.State.ToString())
                {
                    RecordType = ProgressRecordType.Processing,
                };

                if (progress.State == PackageInstallProgressState.Downloading && progress.BytesRequired != 0)
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
                adapter.WriteProgress(new ProgressRecord(1, activity, status.ToString())
                {
                    RecordType = ProgressRecordType.Completed,
                });
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
