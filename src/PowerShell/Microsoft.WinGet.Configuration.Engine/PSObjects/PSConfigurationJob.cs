// -----------------------------------------------------------------------------
// <copyright file="PSConfigurationJob.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using System.Threading.Tasks;
    using Microsoft.WinGet.Common.Command;

    /// <summary>
    /// This is a wrapper object for asynchronous task for this module.
    /// Contains the necessary information to continue the operation.
    /// </summary>
    public class PSConfigurationJob
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSConfigurationJob"/> class.
        /// </summary>
        /// <param name="applyConfigTask">The apply configuration task.</param>
        /// <param name="startCommand">The start command.</param>
        internal PSConfigurationJob(
            Task<PSApplyConfigurationSetResult> applyConfigTask,
            PowerShellCmdlet startCommand)
        {
            this.ApplyConfigurationTask = applyConfigTask;
            this.StartCommand = startCommand;
        }

        /// <summary>
        /// Gets the running configuration task.
        /// </summary>
        internal Task<PSApplyConfigurationSetResult> ApplyConfigurationTask { get; private set; }

        /// <summary>
        /// Gets the command that started async operation.
        /// </summary>
        internal PowerShellCmdlet StartCommand { get; private set; }
    }
}
