// -----------------------------------------------------------------------------
// <copyright file="CliCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Common.Command;

    /// <summary>
    /// Commands that just calls winget.exe underneath.
    /// </summary>
    public sealed class CliCommand : BaseCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="CliCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        public CliCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
        }

        /// <summary>
        /// Enables admin setting.
        /// </summary>
        /// <param name="name">Setting name.</param>
        public void EnableSetting(string name)
        {
            Utilities.VerifyAdmin();
            _ = this.Run("settings", $"--enable {name}");
        }

        /// <summary>
        /// Disables admin setting.
        /// </summary>
        /// <param name="name">Setting name.</param>
        public void DisableSetting(string name)
        {
            Utilities.VerifyAdmin();
            _ = this.Run("settings", $"--disable {name}");
        }

        /// <summary>
        /// Gets winget settings.
        /// </summary>
        /// <param name="asPlainText">Return as string.</param>
        public void GetSettings(bool asPlainText)
        {
            var result = this.Run("settings", "export");

            if (asPlainText)
            {
                this.Write(StreamType.Object, result.StdOut);
            }
            else
            {
                this.Write(StreamType.Object, Utilities.ConvertToHashtable(result.StdOut));
            }
        }

        /// <summary>
        /// Adds source.
        /// </summary>
        /// <param name="name">Name of source.</param>
        /// <param name="arg">Arg of source.</param>
        /// <param name="type">Type of source.</param>
        public void AddSource(string name, string arg, string type)
        {
            Utilities.VerifyAdmin();
            if (string.IsNullOrEmpty(type))
            {
                _ = this.Run("source", $"add --name {name} --arg {arg}", 300000);
            }
            else
            {
                _ = this.Run("source", $"add --name {name} --arg {arg} --type {type}", 300000);
            }
        }

        /// <summary>
        /// Removes source.
        /// </summary>
        /// <param name="name">Name of source.</param>
        public void RemoveSource(string name)
        {
            Utilities.VerifyAdmin();
            _ = this.Run("source", $"remove --name {name}");
        }

        /// <summary>
        /// Resets source.
        /// </summary>
        /// <param name="name">Name of source.</param>
        public void ResetSource(string name)
        {
            Utilities.VerifyAdmin();
            _ = this.Run("source", $"reset --name {name} --force");
        }

        private WinGetCLICommandResult Run(string command, string parameters, int timeOut = 60000)
        {
            var wingetCliWrapper = new WingetCLIWrapper();
            var result = wingetCliWrapper.RunCommand(command, parameters, timeOut);
            result.VerifyExitCode();

            return result;
        }
    }
}
