// -----------------------------------------------------------------------------
// <copyright file="WinGetCLICommandBuilder.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System.Collections.Generic;
    using System.Text;

    /// <summary>
    /// Represents a builder for WinGet CLI commands.
    /// </summary>
    public class WinGetCLICommandBuilder
    {
        private readonly List<string> commands;
        private readonly List<string> parameters;

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetCLICommandBuilder"/> class.
        /// </summary>
        /// <param name="commands">The commands to initialize the builder with.</param>
        public WinGetCLICommandBuilder(params string[] commands)
        {
            this.commands = new (commands);
            this.parameters = new ();
        }

        /// <summary>
        /// Gets the main command (e.g. [empty], settings, source).
        /// </summary>
        public string Command => string.Join(" ", this.commands);

        /// <summary>
        /// Gets the constructed parameters string.
        /// </summary>
        public string Parameters => string.Join(" ", this.parameters);

        /// <summary>
        /// Appends a switch to the command.
        /// </summary>
        /// <param name="switchName">The name of the switch to append.</param>
        /// <returns>The current instance of <see cref="WinGetCLICommandBuilder"/>.</returns>
        public WinGetCLICommandBuilder AppendSwitch(string switchName)
        {
            this.parameters.Add($"--{switchName}");
            return this;
        }

        /// <summary>
        /// Appends a sub-command to the command.
        /// </summary>
        /// <param name="subCommand">The sub-command to append.</param>
        /// <returns>The current instance of <see cref="WinGetCLICommandBuilder"/>.</returns>
        public WinGetCLICommandBuilder AppendSubCommand(string subCommand)
        {
            this.commands.Add(subCommand);
            return this;
        }

        /// <summary>
        /// Appends an option with its value to the command.
        /// </summary>
        /// <param name="option">The name of the option to append.</param>
        /// <param name="value">The value of the option to append.</param>
        /// <returns>The current instance of <see cref="WinGetCLICommandBuilder"/>.</returns>
        public WinGetCLICommandBuilder AppendOption(string option, string? value)
        {
            if (value == null)
            {
                return this;
            }

            this.parameters.Add($"--{option} {this.Escape(value)}");
            return this;
        }

        /// <summary>
        /// Converts the command builder to its string representation.
        /// </summary>
        /// <returns>The string representation of the command.</returns>
        public override string ToString()
        {
            var parametersString = this.Parameters;
            var commandString = this.Command;
            if (string.IsNullOrEmpty(commandString))
            {
                return parametersString;
            }

            if (string.IsNullOrEmpty(parametersString))
            {
                return commandString;
            }

            return $"{commandString} {parametersString}";
        }

        /// <summary>
        /// Escapes a command-line argument according to Windows command-line parsing rules.
        /// References:
        /// - https://devblogs.microsoft.com/oldnewthing/20100917-00/?p=12833
        /// - https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments.
        /// </summary>
        /// <param name="arg">The argument to escape.</param>
        /// <returns>The escaped argument.</returns>
        private string Escape(string arg)
        {
            if (string.IsNullOrEmpty(arg))
            {
                return "\"\"";
            }

            var sb = new StringBuilder(arg.Length + 2);
            sb.Append('"');

            int bs = 0;
            foreach (char c in arg)
            {
                if (c == '\\')
                {
                    bs++;
                }
                else if (c == '"')
                {
                    sb.Append('\\', (bs * 2) + 1);
                    sb.Append('"');
                    bs = 0;
                }
                else
                {
                    sb.Append('\\', bs);
                    sb.Append(c);
                    bs = 0;
                }
            }

            if (bs > 0)
            {
                sb.Append('\\', bs * 2);
            }

            sb.Append('"');
            return sb.ToString();
        }
    }
}
