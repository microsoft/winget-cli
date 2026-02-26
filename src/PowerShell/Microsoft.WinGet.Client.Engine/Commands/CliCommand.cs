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
            _ = this.Run(new WinGetCLICommandBuilder("settings").AppendOption("enable", name));
        }

        /// <summary>
        /// Disables admin setting.
        /// </summary>
        /// <param name="name">Setting name.</param>
        public void DisableSetting(string name)
        {
            Utilities.VerifyAdmin();
            _ = this.Run(new WinGetCLICommandBuilder("settings").AppendOption("disable", name));
        }

        /// <summary>
        /// Gets winget settings.
        /// </summary>
        /// <param name="asPlainText">Return as string.</param>
        public void GetSettings(bool asPlainText)
        {
            var result = this.Run(new WinGetCLICommandBuilder("settings").AppendSubCommand("export"));

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
        /// <param name="trustLevel">Trust level of source.</param>
        /// <param name="isExplicit">Make source explicit.</param>
        /// <param name="priority">Set the priority if the source.</param>
        public void AddSource(string name, string arg, string type, string trustLevel, bool isExplicit, int priority)
        {
            Utilities.VerifyAdmin();
            var builder = new WinGetCLICommandBuilder("source")
                .AppendSubCommand("add")
                .AppendOption("name", name)
                .AppendOption("arg", arg);

            if (!string.IsNullOrEmpty(type))
            {
                builder.AppendOption("type", type);
            }

            if (!string.IsNullOrEmpty(trustLevel))
            {
                builder.AppendOption("trust-level", trustLevel);
            }

            if (isExplicit)
            {
                builder.AppendSwitch("explicit");
            }

            if (priority != 0)
            {
                builder.AppendOption("priority", priority.ToString());
            }

            _ = this.Run(builder, 300000);
        }

        /// <summary>
        /// Removes source.
        /// </summary>
        /// <param name="name">Name of source.</param>
        public void RemoveSource(string name)
        {
            Utilities.VerifyAdmin();
            _ = this.Run(new WinGetCLICommandBuilder("source").AppendSubCommand("remove").AppendOption("name", name));
        }

        /// <summary>
        /// Resets a source.
        /// </summary>
        /// <param name="name">Name of source.</param>
        public void ResetSourceByName(string name)
        {
            Utilities.VerifyAdmin();
            _ = this.Run(new WinGetCLICommandBuilder("source").AppendSubCommand("reset").AppendOption("name", name).AppendSwitch("force"));
        }

        /// <summary>
        /// Resets all sources and adds the defaults.
        /// </summary>
        public void ResetAllSources()
        {
            Utilities.VerifyAdmin();
            _ = this.Run(new WinGetCLICommandBuilder("source").AppendSubCommand("reset").AppendSwitch("force"));
        }

        private WinGetCLICommandResult Run(WinGetCLICommandBuilder builder, int timeOut = 60000)
        {
            var wingetCliWrapper = new WingetCLIWrapper();
            var result = wingetCliWrapper.RunCommand(this, builder, timeOut);
            result.VerifyExitCode();

            return result;
        }
    }
}
