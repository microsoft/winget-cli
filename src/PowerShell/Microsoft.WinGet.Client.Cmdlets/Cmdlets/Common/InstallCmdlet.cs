// -----------------------------------------------------------------------------
// <copyright file="InstallCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands.Common
{
    using System.IO;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.PSObjects;

    /// <summary>
    /// This is the base class for all commands that parse a FindPackagesOptions result
    /// from the provided parameters i.e., the "install" and "upgrade" commands.
    /// </summary>
    public abstract class InstallCmdlet : InstallerSelectionCmdlet
    {
        private string location;

        /// <summary>
        /// Gets or sets the mode to manipulate the package with.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public PSPackageInstallMode Mode { get; set; } = PSPackageInstallMode.Default;

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
        /// Gets or sets the path to the logging file.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Log { get; set; }

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
    }
}
