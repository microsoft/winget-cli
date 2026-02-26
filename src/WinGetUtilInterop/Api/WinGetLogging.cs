// -----------------------------------------------------------------------------
// <copyright file="WinGetLogging.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Api
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using Microsoft.WinGetUtil.Common;
    using Microsoft.WinGetUtil.Interfaces;

    /// <summary>
    /// Wrapper class around WinGetUtil log native implementation.
    /// For dll entry points are defined here:
    ///     https://github.com/microsoft/winget-cli/blob/master/src/WinGetUtil/WinGetUtil.h.
    /// </summary>
    public sealed class WinGetLogging : IWinGetLogging
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetLogging"/> class.
        /// </summary>
        /// <param name="logFile">Open index file. Must be closed at dispose.</param>
        internal WinGetLogging(string logFile)
        {
            this.LogFile = logFile;
        }

        /// <summary>
        /// Gets log file.
        /// </summary>
        public string LogFile { get; private set; }

        /// <summary>
        /// Dispose method.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Dispose method to dispose the Git desktop process runner.
        /// </summary>
        /// <param name="disposing">Bool value indicating if Dispose is being run.</param>
        public void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (File.Exists(this.LogFile))
                {
                    WinGetLoggingTerm(this.LogFile);
                }
            }
        }

        //// <summary>
        //// Closes the index log file.
        //// </summary>
        //// <param name="logPath">Log path to close. Null for all.</param>
        //// <returns>HRESULT.</returns>
        [DllImport(Constants.DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        private static extern IntPtr WinGetLoggingTerm(string logPath);
    }
}
