// -----------------------------------------------------------------------------
// <copyright file="ProcessExecution.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Text;
    using System.Threading;
    using Microsoft.Management.Configuration.Processor.Helpers;

    /// <summary>
    /// Wrapper for a single process execution and its output.
    /// </summary>
    internal class ProcessExecution
    {
        private List<string> outputLines = new List<string>();
        private List<string> errorLines = new List<string>();

        /// <summary>
        /// Initializes a new instance of the <see cref="ProcessExecution"/> class.
        /// </summary>
        public ProcessExecution()
        {
        }

        /// <summary>
        /// An event that receives the output lines as they are delivered.
        /// </summary>
        public event EventHandler<string>? OutputLineReceived;

        /// <summary>
        /// An event that receives the error lines as they are delivered.
        /// </summary>
        public event EventHandler<string>? ErrorLineReceived;

        /// <summary>
        /// Gets the executable path.
        /// </summary>
        required public string ExecutablePath { get; init; }

        /// <summary>
        /// Gets the arguments to use for the process.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1010:Opening square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
        public IEnumerable<string> Arguments { get; init; } = [];

        /// <summary>
        /// Gets the data to write to standard input of the process.
        /// </summary>
        public string? Input { get; init; } = null;

        /// <summary>
        /// Gets the list of custom environment variables to use for the process.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1010:Opening square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
        public IEnumerable<ProcessExecutionEnvironmentVariable> EnvironmentVariables { get; init; } = [];

        /// <summary>
        /// Gets the argument string passed to the process.
        /// </summary>
        public string SerializedArguments
        {
            get
            {
                StringBuilder processArguments = new StringBuilder();

                foreach (string arg in this.Arguments)
                {
                    if (processArguments.Length != 0)
                    {
                        processArguments.Append(' ');
                    }

                    processArguments.Append(arg);
                }

                return processArguments.ToString();
            }
        }

        /// <summary>
        /// Gets the full command line that the process should see.
        /// </summary>
        public string CommandLine
        {
            get
            {
                return $"{this.ExecutablePath} {this.SerializedArguments}";
            }
        }

        /// <summary>
        /// Gets the current set of output lines.
        /// Not thread safe, use OutputLineReceived for async flows.
        /// </summary>
        public IReadOnlyCollection<string> Output
        {
            get { return this.outputLines; }
        }

        /// <summary>
        /// Gets the current set of error lines.
        /// Not thread safe, use ErrorLineReceived for async flows.
        /// </summary>
        public IReadOnlyCollection<string> Error
        {
            get { return this.errorLines; }
        }

        /// <summary>
        /// Gets the exit code of the process.
        /// Will be null until the process exits.
        /// </summary>
        public int? ExitCode { get; private set; } = null;

        /// <summary>
        /// Gets or sets the process object; null until Start called.
        /// </summary>
        private Process? Process { get; set; }

        /// <summary>
        /// Starts the process.
        /// </summary>
        /// <returns>This object.</returns>
        /// <exception cref="InvalidOperationException">Thrown if Start has already been called.</exception>
        public ProcessExecution Start()
        {
            if (this.Process != null)
            {
                throw new InvalidOperationException("Process has already been started.");
            }

            ProcessStartInfo startInfo;

            lock (PathEnvironmentVariableHandler.Lock)
            {
                startInfo = new ProcessStartInfo(this.ExecutablePath, this.SerializedArguments);
            }

            this.Process = new Process() { StartInfo = startInfo };

            startInfo.UseShellExecute = false;
            startInfo.WindowStyle = ProcessWindowStyle.Hidden;

            startInfo.StandardOutputEncoding = Encoding.UTF8;
            startInfo.RedirectStandardOutput = true;
            this.Process.OutputDataReceived += (sender, args) =>
            {
                string? output = args.Data;

                if (output != null)
                {
                    this.outputLines.Add(output);

                    this.OutputLineReceived?.Invoke(this, output);
                }
            };

            startInfo.StandardErrorEncoding = Encoding.UTF8;
            startInfo.RedirectStandardError = true;
            this.Process.ErrorDataReceived += (sender, args) =>
            {
                string? error = args.Data;

                if (error != null)
                {
                    this.errorLines.Add(error);

                    this.ErrorLineReceived?.Invoke(this, error);
                }
            };

            if (this.Input != null)
            {
                startInfo.StandardInputEncoding = Encoding.UTF8;
                startInfo.RedirectStandardInput = true;
            }

            foreach (var env in this.EnvironmentVariables)
            {
                switch (env.ValueType)
                {
                    case ProcessExecutionEnvironmentVariableValueType.Override:
                        startInfo.EnvironmentVariables[env.Name] = env.Value;
                        break;

                    case ProcessExecutionEnvironmentVariableValueType.Prepend:
                        startInfo.EnvironmentVariables[env.Name] = MergeStringsWithSeparator(env.Value, startInfo.EnvironmentVariables[env.Name] ?? string.Empty, env.Separator);
                        break;

                    case ProcessExecutionEnvironmentVariableValueType.Append:
                        startInfo.EnvironmentVariables[env.Name] = MergeStringsWithSeparator(startInfo.EnvironmentVariables[env.Name] ?? string.Empty, env.Value, env.Separator);
                        break;
                }
            }

            this.Process.Start();
            this.Process.BeginOutputReadLine();
            this.Process.BeginErrorReadLine();

            if (this.Input != null)
            {
                this.Process.StandardInput.Write(this.Input);
                this.Process.StandardInput.Close();
            }

            return this;
        }

        /// <summary>
        /// Waits for the process to exit.
        /// </summary>
        /// <param name="milliseconds">The minimum amount of time to wait for the process to exit, in milliseconds.</param>
        /// <returns>True if the process exited; false if not.</returns>
        /// <exception cref="InvalidOperationException">Thrown if Start has not been called.</exception>
        public bool WaitForExit(int milliseconds = Timeout.Infinite)
        {
            if (this.Process == null)
            {
                throw new InvalidOperationException("Process has not been started.");
            }

            if (this.Process.WaitForExit(milliseconds))
            {
                // According to documentation, this extra call will ensure that the redirected streams have finished reading all of the data.
                this.Process.WaitForExit();

                this.ExitCode = this.Process.ExitCode;

                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Gets all of the output lines as a single string.
        /// </summary>
        /// <returns>The output lines as a string.</returns>
        public string GetAllOutputLines()
        {
            return GetAllLines(this.outputLines);
        }

        /// <summary>
        /// Gets all of the error lines as a single string.
        /// </summary>
        /// <returns>The error lines as a string.</returns>
        public string GetAllErrorLines()
        {
            return GetAllLines(this.errorLines);
        }

        private static string GetAllLines(List<string> lines)
        {
            StringBuilder stringBuilder = new StringBuilder();

            foreach (string line in lines)
            {
                stringBuilder.AppendLine(line);
            }

            return stringBuilder.ToString();
        }

        private static string MergeStringsWithSeparator(string first, string second, string separator)
        {
            if (string.IsNullOrEmpty(separator))
            {
                return first + second;
            }
            else
            {
                if (first.EndsWith(separator) && second.StartsWith(separator))
                {
                    return first + second.Substring(separator.Length);
                }
                else if (first.EndsWith(separator) || second.StartsWith(separator))
                {
                    return first + second;
                }
                else
                {
                    return first + separator + second;
                }
            }
        }
    }
}
