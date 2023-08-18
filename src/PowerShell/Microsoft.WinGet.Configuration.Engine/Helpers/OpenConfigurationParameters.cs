// -----------------------------------------------------------------------------
// <copyright file="OpenConfigurationParameters.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Helpers
{
    using System;
    using System.IO;
    using System.Management.Automation;
    using Microsoft.Management.Configuration.Processor;
    using Microsoft.PowerShell;
    using Microsoft.WinGet.Configuration.Engine.Resources;

    /// <summary>
    /// The parameters used to open a configuration.
    /// </summary>
    internal class OpenConfigurationParameters
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="OpenConfigurationParameters"/> class.
        /// </summary>
        /// <param name="psCmdlet">PsCmdlet caller.</param>
        /// <param name="file">The configuration file.</param>
        /// <param name="allUsers">If use all users location.</param>
        /// <param name="currentUser">If use current user location.</param>
        /// <param name="customLocation">The custom location.</param>
        /// <param name="executionPolicy">Execution policy.</param>
        /// <param name="canUseTelemetry">If telemetry can be used.</param>
        public OpenConfigurationParameters(
            PSCmdlet psCmdlet,
            string file,
            bool allUsers,
            bool currentUser,
            string customLocation,
            ExecutionPolicy executionPolicy,
            bool canUseTelemetry)
        {
            this.ConfigFile = this.VerifyFile(file, psCmdlet);

            if (Convert.ToInt32(allUsers) + Convert.ToInt32(currentUser) + Convert.ToInt32(!string.IsNullOrEmpty(customLocation)) > 1)
            {
                throw new ArgumentException(Resources.ConfigurationLocationExclusive);
            }

            if (allUsers)
            {
                if (!Utilities.ExecutingAsAdministrator)
                {
                    throw new ArgumentException(Resources.ConfigurationAllUsersElevated);
                }

                this.Location = PowerShellConfigurationProcessorLocation.AllUsers;
            }
            else if (!string.IsNullOrEmpty(customLocation))
            {
                this.CustomLocation = customLocation;
                this.Location = PowerShellConfigurationProcessorLocation.Custom;
            }
            else
            {
                // TODO: read from the global object.
                this.Location = PowerShellConfigurationProcessorLocation.CurrentUser;
            }

            this.Policy = this.GetConfigurationProcessorPolicy(executionPolicy);

            this.CanUseTelemetry = canUseTelemetry;
        }

        /// <summary>
        /// Gets the configuration file.
        /// </summary>
        public string ConfigFile { get; }

        /// <summary>
        /// Gets the location to install the modules.
        /// </summary>
        public PowerShellConfigurationProcessorLocation Location { get; }

        /// <summary>
        /// Gets the custom location to install modules.
        /// </summary>
        public string? CustomLocation { get; }

        /// <summary>
        /// Gets execution policy of the processor.
        /// </summary>
        public PowerShellConfigurationProcessorPolicy Policy { get; }

        /// <summary>
        /// Gets a value indicating whether to use telemetry or not.
        /// </summary>
        public bool CanUseTelemetry { get; }

        private string VerifyFile(string filePath, PSCmdlet psCmdlet)
        {
            if (!Path.IsPathRooted(filePath))
            {
                filePath = Path.GetFullPath(
                    Path.Combine(
                        psCmdlet.SessionState.Path.CurrentFileSystemLocation.Path,
                        filePath));
            }

            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException(filePath);
            }

            return filePath;
        }

        private PowerShellConfigurationProcessorPolicy GetConfigurationProcessorPolicy(ExecutionPolicy policy)
        {
            return policy switch
            {
                ExecutionPolicy.Unrestricted => PowerShellConfigurationProcessorPolicy.Unrestricted,
                ExecutionPolicy.RemoteSigned => PowerShellConfigurationProcessorPolicy.RemoteSigned,
                ExecutionPolicy.AllSigned => PowerShellConfigurationProcessorPolicy.AllSigned,
                ExecutionPolicy.Restricted => PowerShellConfigurationProcessorPolicy.Restricted,
                ExecutionPolicy.Bypass => PowerShellConfigurationProcessorPolicy.Bypass,
                _ => throw new InvalidOperationException(),
            };
        }
    }
}
