// -----------------------------------------------------------------------------
// <copyright file="PSConfigurationJob.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using System.Threading.Tasks;
    using Microsoft.WinGet.Configuration.Engine.Commands;

    /// <summary>
    /// This is a wrapper object for asynchronous task for this module.
    /// Contains the necessary information to continue the operation.
    /// </summary>
    public class PSConfigurationJob
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSConfigurationJob"/> class.
        /// </summary>
        /// <param name="configTask">The configuration task.</param>
        /// <param name="startCommand">The start command.</param>
        internal PSConfigurationJob(
            Task<PSConfigurationSet> configTask,
            AsyncCommand startCommand)
        {
            this.ConfigurationTask = configTask;
            this.StartCommand = startCommand;
        }

        /// <summary>
        /// Gets the running configuration task.
        /// </summary>
        internal Task<PSConfigurationSet> ConfigurationTask { get; private set; }

        /// <summary>
        /// Gets the command that started async operation.
        /// </summary>
        internal AsyncCommand StartCommand { get; private set; }

        /// <summary>
        /// Gets the status of the configuration task.
        /// </summary>
        /// <returns>The task status.</returns>
        public string GetStatus()
        {
            return this.ConfigurationTask.Status.ToString();
        }
    }
}
