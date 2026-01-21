// -----------------------------------------------------------------------------
// <copyright file="WinGetCLICommandBuilder.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.Linq;
    using System.Text;

    /// <summary>
    /// Represents a builder for WinGet CLI commands.
    /// </summary>
    public class WinGetCLICommandBuilder
    {
        private readonly StringBuilder builder;

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetCLICommandBuilder"/> class.
        /// </summary>
        /// <param name="command">Optional main command (e.g., settings, source).</param>
        public WinGetCLICommandBuilder(string? command = null)
        {
            this.builder = new StringBuilder();
            this.Command = command;
        }

        /// <summary>
        /// Gets the main command (e.g., settings, source).
        /// </summary>
        public string? Command { get; }

        /// <summary>
        /// Gets the constructed parameters string.
        /// </summary>
        public string Parameters => this.builder.ToString();

        /// <summary>
        /// Appends a switch to the command.
        /// </summary>
        /// <param name="switchName">The name of the switch to append.</param>
        /// <returns>The current instance of <see cref="WinGetCLICommandBuilder"/>.</returns>
        public WinGetCLICommandBuilder AppendSwitch(string switchName)
        {
            this.AppendToken($"--{switchName}");
            return this;
        }

        /// <summary>
        /// Appends a sub-command to the command.
        /// </summary>
        /// <param name="subCommand">The sub-command to append.</param>
        /// <returns>The current instance of <see cref="WinGetCLICommandBuilder"/>.</returns>
        public WinGetCLICommandBuilder AppendSubCommand(string subCommand)
        {
            this.AppendToken(subCommand);
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

            this.AppendToken($"--{option} {this.Escape(value)}");
            return this;
        }

        /// <summary>
        /// Converts the command builder to its string representation.
        /// </summary>
        /// <returns>The string representation of the command.</returns>
        public override string ToString()
        {
            if (string.IsNullOrEmpty(this.Command))
            {
                return this.Parameters;
            }

            if (string.IsNullOrEmpty(this.Parameters))
            {
                return this.Command!;
            }

            return $"{this.Command} {this.Parameters}";
        }

        /// <summary>
        /// Escapes a command-line argument according to Windows command-line parsing rules.
        /// </summary>
        /// <param name="arg">The argument to escape.</param>
        /// <returns>The escaped argument.</returns>
        private string Escape(string arg)
        {
            if (string.IsNullOrEmpty(arg))
            {
                return "\"\"";
            }

            // Determine if the argument needs quotes
            // - Contains whitespace, or
            // - Contains double quotes, or
            // - Ends with a backslash
            var needsQuotes = arg.Any(char.IsWhiteSpace) || arg.Contains('"') || arg.EndsWith("\\", StringComparison.Ordinal);
            if (!needsQuotes)
            {
                return arg;
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

        /// <summary>
        /// Appends a token to the command.
        /// </summary>
        /// <param name="token">The token to append.</param>
        private void AppendToken(string token)
        {
            if (this.builder.Length > 0)
            {
                this.builder.Append(' ');
            }

            this.builder.Append(token);
        }
    }
}
