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
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// The parameters used to open a configuration.
    /// </summary>
    internal class OpenConfigurationParameters
    {
        private const string Default = "default";
        private const string AllUsers = "allusers";
        private const string CurrentUser = "currentuser";

        /// <summary>
        /// Initializes a new instance of the <see cref="OpenConfigurationParameters"/> class.
        /// </summary>
        /// <param name="pwshCmdlet">PowerShellCmdlet.</param>
        /// <param name="file">The configuration file.</param>
        /// <param name="modulePath">The module path to use.</param>
        /// <param name="executionPolicy">Execution policy.</param>
        /// <param name="canUseTelemetry">If telemetry can be used.</param>
        /// <param name="fromHistory">If the configuration is from history; changes the meaning of `ConfigFile` to the instance identifier.</param>
        public OpenConfigurationParameters(
            PowerShellCmdlet pwshCmdlet,
            string file,
            string modulePath,
            ExecutionPolicy executionPolicy,
            bool canUseTelemetry,
            bool fromHistory = false)
        {
            if (!fromHistory)
            {
                this.ConfigFile = this.VerifyFile(file, pwshCmdlet);
            }
            else
            {
                this.ConfigFile = file;
            }

            this.InitializeModulePath(modulePath);
            this.Policy = this.GetConfigurationProcessorPolicy(executionPolicy);
            this.CanUseTelemetry = canUseTelemetry;
            this.FromHistory = fromHistory;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="OpenConfigurationParameters"/> class.
        /// </summary>
        /// <param name="pwshCmdlet">PowerShellCmdlet.</param>
        /// <param name="modulePath">The module path to use.</param>
        /// <param name="executionPolicy">Execution policy.</param>
        /// <param name="canUseTelemetry">If telemetry can be used.</param>
        public OpenConfigurationParameters(
            PowerShellCmdlet pwshCmdlet,
            string modulePath,
            ExecutionPolicy executionPolicy,
            bool canUseTelemetry)
        {
            this.ConfigFile = string.Empty;
            this.InitializeModulePath(modulePath);
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
        public PowerShellConfigurationProcessorLocation Location { get; private set; }

        /// <summary>
        /// Gets the custom location to install modules.
        /// </summary>
        public string? CustomLocation { get; private set; }

        /// <summary>
        /// Gets execution policy of the processor.
        /// </summary>
        public PowerShellConfigurationProcessorPolicy Policy { get; }

        /// <summary>
        /// Gets a value indicating whether to use telemetry or not.
        /// </summary>
        public bool CanUseTelemetry { get; }

        /// <summary>
        /// Gets a value indicating whether the configuration is from history.
        /// </summary>
        public bool FromHistory { get; }

        private string VerifyFile(string filePath, PowerShellCmdlet pwshCmdlet)
        {
            if (!Path.IsPathRooted(filePath))
            {
                filePath = Path.GetFullPath(
                    Path.Combine(
                        pwshCmdlet.GetCurrentFileSystemLocation(),
                        filePath));
            }
            else
            {
                filePath = Path.GetFullPath(filePath);
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

        private void InitializeModulePath(string modulePath)
        {
            // TODO: Create cmdlet that specify the global custom location for a PowerShell session.
            if (!string.IsNullOrEmpty(modulePath))
            {
                if (string.Compare(modulePath, Default, true) == 0)
                {
                    this.Location = PowerShellConfigurationProcessorLocation.Default;
                }
                else if (string.Compare(modulePath, CurrentUser, true) == 0)
                {
                    this.Location = PowerShellConfigurationProcessorLocation.CurrentUser;
                }
                else if (string.Compare(modulePath, AllUsers, true) == 0)
                {
                    if (!Utilities.ExecutingAsAdministrator)
                    {
                        throw new ArgumentException(Resources.ConfigurationAllUsersElevated);
                    }

                    this.Location = PowerShellConfigurationProcessorLocation.AllUsers;
                }
                else
                {
                    string customLocation = modulePath;
                    if (!Path.IsPathRooted(customLocation))
                    {
                        throw new ArgumentException(Resources.ConfigurationModulePathArgError);
                    }

                    this.CustomLocation = Path.GetFullPath(customLocation);
                    this.Location = PowerShellConfigurationProcessorLocation.Custom;
                }
            }
            else
            {
                this.Location = PowerShellConfigurationProcessorLocation.WinGetModulePath;
            }
        }
    }
}
