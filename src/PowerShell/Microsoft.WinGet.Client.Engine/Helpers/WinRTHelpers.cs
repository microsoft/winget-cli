// -----------------------------------------------------------------------------
// <copyright file="WinRTHelpers.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Helper class for winrtact.dll calls.
    /// </summary>
    internal static class WinRTHelpers
    {
#if POWERSHELL_WINDOWS
        private static readonly string ArchDependencyPath;

        static WinRTHelpers()
        {
            ArchDependencyPath = Path.Combine(
                    Path.GetDirectoryName(
                        Path.GetDirectoryName(typeof(WinRTHelpers).Assembly.Location)),
                    "SharedDependencies",
                    RuntimeInformation.ProcessArchitecture.ToString().ToLower());
        }
#endif

        /// <summary>
        /// Calls winrtact_Initialize.
        /// </summary>
        public static void Initialize()
        {
#if POWERSHELL_WINDOWS
            SetDllDirectoryW(ArchDependencyPath);

            try
            {
#endif
                InitializeUndockedRegFreeWinRT();

#if POWERSHELL_WINDOWS
            }
            finally
            {
                SetDllDirectoryW(null);
            }
#endif
        }

        /// <summary>
        /// Calls WinGetServerManualActivation_CreateInstance.
        /// </summary>
        /// <param name="clsid">Class id.</param>
        /// <param name="iid">IID.</param>
        /// <param name="flags">Flags.</param>
        /// <param name="instance">Out object.</param>
        /// <returns>Result of WinGetServerManualActivation_CreateInstance.</returns>
        public static int ManualActivation(Guid clsid, Guid iid, uint flags, out object instance)
        {
#if POWERSHELL_WINDOWS
            SetDllDirectoryW(ArchDependencyPath);

            try
            {
#endif
                return WinGetServerManualActivation_CreateInstance(clsid, iid, flags, out instance);

#if POWERSHELL_WINDOWS
            }
            finally
            {
                SetDllDirectoryW(null);
            }
#endif
        }

        [DllImport("winrtact.dll", EntryPoint = "winrtact_Initialize", ExactSpelling = true, PreserveSig = true)]
        private static extern void InitializeUndockedRegFreeWinRT();

        [DllImport("winrtact.dll", EntryPoint = "WinGetServerManualActivation_CreateInstance", ExactSpelling = true, PreserveSig = true)]
        private static extern int WinGetServerManualActivation_CreateInstance(
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid clsid,
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid iid,
            uint flags,
            [Out, MarshalAs(UnmanagedType.IUnknown)] out object instance);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool SetDllDirectoryW([MarshalAs(UnmanagedType.LPWStr)] string? directory);
    }
}
