// -----------------------------------------------------------------------------
// <copyright file="WinGetServerInstance.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetTestCommon
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Represents an instance of a Windows Package Manager (WinGet) server.
    /// </summary>
    public class WinGetServerInstance
    {
        /// <summary>
        /// The name of the executable for the COM server.
        /// </summary>
        public const string ServerExecutableName = "WindowsPackageManagerServer";

        /// <summary>
        /// The package family name for the development package.
        /// </summary>
        public const string DevelopmentPackageFamilyName = "WinGetDevCLI_8wekyb3d8bbwe";

        /// <summary>
        /// The window name for the COM server message window.
        /// </summary>
        public const string TargetWindowName = "WingetMessageOnlyWindow";

        /// <summary>
        /// Gets the process for the server.
        /// </summary>
        public required Process Process { get; init; }

        /// <summary>
        /// Gets a value indicating whether the current server has an associated window.
        /// </summary>
        public bool HasWindow
        {
            get
            {
                return EnumerateWindowHandles(TargetWindowName).Count > 0;
            }
        }

        /// <summary>
        /// Sends a specified message to a window.
        /// </summary>
        /// <param name="message">The message to be sent to the window.</param>
        /// <returns>True to indicate that the message was sent and processed within the timeout; false otherwise.</returns>
        public bool SendMessage(WindowMessage message)
        {
            const int TRUE = 0x1;
            const int ENDSESSION_CLOSEAPP = 0x1;
            const uint SMTO_ABORTIFHUNG = 0x0002;
            const uint TIMEOUT_MS = 5000;

            var windowHandles = EnumerateWindowHandles(TargetWindowName);

            if (windowHandles.Count > 1)
            {
                throw new InvalidOperationException($"Target process has more than one window named `{TargetWindowName}`");
            }

            foreach (var hWnd in windowHandles)
            {
                IntPtr result;
                bool success;
                switch (message)
                {
                    case WindowMessage.Close:
                        success = SendMessageTimeout(hWnd, (uint)message, IntPtr.Zero, IntPtr.Zero, SMTO_ABORTIFHUNG, TIMEOUT_MS, out result) != IntPtr.Zero;
                        break;
                    case WindowMessage.QueryEndSession:
                        success = SendMessageTimeout(hWnd, (uint)message, IntPtr.Zero, (IntPtr)ENDSESSION_CLOSEAPP, SMTO_ABORTIFHUNG, TIMEOUT_MS, out result) != IntPtr.Zero;
                        break;
                    case WindowMessage.EndSession:
                        success = SendMessageTimeout(hWnd, (uint)message, (IntPtr)TRUE, (IntPtr)ENDSESSION_CLOSEAPP, SMTO_ABORTIFHUNG, TIMEOUT_MS, out result) != IntPtr.Zero;
                        break;
                    default:
                        throw new NotImplementedException("Unexpected window message");
                }

                return success;
            }

            return false;
        }

        /// <summary>
        /// Retrieves an array of all available WinGet server instances.
        /// </summary>
        /// <returns>
        /// An array of <see cref="WinGetServerInstance"/> objects representing the available server instances.
        /// The array will be empty if no instances are available.
        /// </returns>
        public static List<WinGetServerInstance> GetInstances()
        {
            Process[] processes = Process.GetProcessesByName(ServerExecutableName);
            List<WinGetServerInstance> result = new List<WinGetServerInstance>();

            foreach (Process process in processes)
            {
                try
                {
                    string? familyName = GetProcessPackageFamilyName(process);
                    if (familyName == DevelopmentPackageFamilyName)
                    {
                        result.Add(new WinGetServerInstance { Process = process });
                    }
                }
                catch
                {
                    // Ignore processes that we can't access or that aren't packaged
                }
            }

            return result;
        }

        private static string? GetProcessPackageFamilyName(Process process)
        {
            const int ERROR_INSUFFICIENT_BUFFER = 122;
            int length = 0;
            int result = GetPackageFamilyName(process.Handle, ref length, null);
            if (result == ERROR_INSUFFICIENT_BUFFER)
            {
                var sb = new System.Text.StringBuilder(length);
                result = GetPackageFamilyName(process.Handle, ref length, sb);
                if (result == 0)
                {
                    return sb.ToString();
                }
            }
            return null;
        }

        private List<IntPtr> EnumerateWindowHandles(string windowName)
        {
            List<IntPtr> windowHandles = new List<IntPtr>();
            int processId = Process.Id;

            bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam)
            {
                GetWindowThreadProcessId(hWnd, out int windowProcessId);
                if (windowProcessId == processId)
                {
                    // Get the window title
                    var sb = new System.Text.StringBuilder(256);
                    int length = GetWindowText(hWnd, sb, sb.Capacity);
                    if (length > 0 && sb.ToString() == windowName)
                    {
                        windowHandles.Add(hWnd);
                    }
                }
                return true;
            }

            EnumWindows(EnumWindowsProc, IntPtr.Zero);
            return windowHandles;
        }

        [DllImport("user32.dll")]
        private static extern bool EnumWindows(EnumWindowsProcDelegate lpEnumFunc, IntPtr lParam);

        private delegate bool EnumWindowsProcDelegate(IntPtr hWnd, IntPtr lParam);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern int GetWindowThreadProcessId(IntPtr hWnd, out int lpdwProcessId);

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder lpString, int nMaxCount);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern IntPtr SendMessageTimeout(
            IntPtr hWnd,
            uint Msg,
            IntPtr wParam,
            IntPtr lParam,
            uint fuFlags,
            uint uTimeout,
            out IntPtr lpdwResult
        );

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern int GetPackageFamilyName(
            IntPtr hProcess,
            ref int packageFamilyNameLength,
            System.Text.StringBuilder? packageFamilyName
        );
    }
}
