// -----------------------------------------------------------------------------
// <copyright file="WingetCLIWrapper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Common.Command;

    /// <summary>
    /// Calls winget directly.
    /// </summary>
    internal class WingetCLIWrapper
    {
        private const int ErrorFileNotFound = 2;

        /// <summary>
        /// The file name to use in start info.
        /// </summary>
        private readonly string wingetPath;

        /// <summary>
        /// Initializes a new instance of the <see cref="WingetCLIWrapper"/> class.
        /// When app execution alias is disabled the path of the exe is
        /// in the package family name directory in the local app data windows app directory. If its enabled then there's
        /// link in the windows app data directory. To avoid checking if its enabled or not, just look in the package
        /// family name directory.
        /// For test, point to the wingetdev executable.
        /// </summary>
        /// <param name="fullPath">Use the full path, or resolve winget from PATH.</param>
        public WingetCLIWrapper(bool fullPath = true)
        {
            if (Utilities.ExecutingAsSystem)
            {
                throw new NotSupportedException();
            }

            if (fullPath)
            {
                this.wingetPath = WinGetFullPath;
            }
            else
            {
                // Search PATH explicitly because Process.Start would also search the current directory.
                this.wingetPath = FindInPath(Environment.GetEnvironmentVariable(Constants.PathEnvVar), Constants.WinGetExe)
                    ?? throw new Win32Exception(ErrorFileNotFound);
            }
        }

        /// <summary>
        /// Gets the full path of winget executable.
        /// </summary>
        public static string WinGetFullPath
        {
            get
            {
                return Path.Combine(
                    Utilities.LocalDataWindowsAppPath,
                    Constants.WingetPackageFamilyName,
                    Constants.WinGetExe);
            }
        }

        /// <summary>
        /// Finds a file in the fully qualified directories of a PATH value, never in the current directory.
        /// </summary>
        /// <param name="envPath">The PATH value.</param>
        /// <param name="fileName">The file name to find.</param>
        /// <returns>The full path of the first match, or null if there is none.</returns>
        internal static string? FindInPath(string? envPath, string fileName)
        {
            foreach (string directory in (envPath ?? string.Empty).Split(Path.PathSeparator))
            {
                // Path.Combine throws on invalid characters in .NET Framework.
                if (IsFullyQualified(directory) && directory.IndexOfAny(Path.GetInvalidPathChars()) < 0)
                {
                    string candidate = Path.Combine(directory, fileName);
                    if (File.Exists(candidate))
                    {
                        return candidate;
                    }
                }
            }

            return null;
        }

        /// <summary>
        /// Runs winget command with parameters.
        /// </summary>
        /// <param name="pwshCmdlet">PowerShell cmdlet.</param>
        /// <param name="builder">The command builder.</param>
        /// <param name="timeOut">Time out.</param>
        /// <returns>WinGetCommandResult.</returns>
        internal WinGetCLICommandResult RunCommand(PowerShellCmdlet pwshCmdlet, WinGetCLICommandBuilder builder, int timeOut = 60000)
        {
            var args = builder.ToString();
            pwshCmdlet.Write(StreamType.Verbose, $"Running {this.wingetPath} with {args}");

            Process p = new ()
            {
                StartInfo = new (this.wingetPath, args)
                {
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                },
            };

            p.Start();

            if (p.WaitForExit(timeOut))
            {
                return new WinGetCLICommandResult(
                    builder.Command,
                    builder.Parameters,
                    p.ExitCode,
                    p.StandardOutput.ReadToEnd(),
                    p.StandardError.ReadToEnd());
            }

            throw new WinGetCLITimeoutException(builder.Command, builder.Parameters);
        }

        // Path.IsPathFullyQualified is not available in .NET Framework.
        private static bool IsFullyQualified(string path)
        {
            if (path.Length < 3)
            {
                return false;
            }

            if (IsDirectorySeparator(path[0]))
            {
                return IsDirectorySeparator(path[1]);
            }

            char drive = path[0];
            bool isDriveLetter = (drive >= 'A' && drive <= 'Z') || (drive >= 'a' && drive <= 'z');
            return isDriveLetter && path[1] == ':' && IsDirectorySeparator(path[2]);
        }

        private static bool IsDirectorySeparator(char c)
        {
            return c == Path.DirectorySeparatorChar || c == Path.AltDirectorySeparatorChar;
        }
    }
}
