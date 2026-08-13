// -----------------------------------------------------------------------------
// <copyright file="RpcSecurityTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Security.Principal;
    using System.Threading;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;
    using CategoryAttribute = NUnit.Framework.CategoryAttribute;

    /// <summary>
    /// RPC manual-activation security tests.
    ///
    /// All tests require the test runner to be elevated (high integrity) and UAC enabled.
    /// They are decorated [Category("RpcSecurity")] and [Explicit] so they are excluded
    /// from default CI runs.
    ///
    /// Run with:
    ///   vstest.console.exe ... --TestCaseFilter:"Category=RpcSecurity"
    ///
    /// Helper exit codes for expect-denial modes (event-signal, mutex-open):
    ///   0 = expected denial occurred (PASS), 1 = unexpected success (FAIL), 2 = error.
    /// Helper exit codes for rpc-connect:
    ///   0 = the call reached the server and succeeded,
    ///   otherwise the HRESULT that rejected the call, expected to be E_ACCESSDENIED.
    /// </summary>
    [TestFixture]
    [Category("RpcSecurity")]
    [Explicit("Requires elevation and UAC-enabled developer machine")]
    public class RpcSecurityTests
    {
#pragma warning disable SA1202 // ElementsMustBeOrderedByAccess
#pragma warning disable SA1307 // AccessibleFieldsMustBeginWithUpperCaseLetter
#pragma warning disable SA1310 // FieldNamesMustNotContainUnderscore
        // Win32 P/Invoke constants
        private const uint Synchronize = 0x00100000;
        private const uint CreateNoWindow = 0x08000000;
        private const uint ExtendedStartupInfoPresent = 0x00080000;
        private const uint ProcessCreateProcess = 0x0080;
        private const uint ProcessQueryLimitedInformation = 0x1000;
        private const uint WaitObject0 = 0x00000000;

        // RPC error codes used in test assertions.
        // ERROR_ACCESS_DENIED / RPC_S_ACCESS_DENIED: the server's endpoint or interface security
        // descriptor rejected the caller, or the client's server security descriptor rejected the server.
        private const int RpcErrorAccessDeniedHResult = -2147024891;

        // PROC_THREAD_ATTRIBUTE_PARENT_PROCESS = ProcThreadAttributeValue(0, FALSE, TRUE, FALSE)
        // = (0 & 0x0000FFFF) | (0 << 16) | (1 << 17) = 0x00020000
        private static readonly IntPtr ProcThreadAttributeParentProcess = new IntPtr(0x00020000);

        /// <summary>Gets the full path to WindowsPackageManagerServer.exe.</summary>
        private string serverPath;

        /// <summary>Gets the full path to WinGetRpcTestHelper.exe.</summary>
        private string helperPath;

        // P/Invoke declarations
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern IntPtr OpenEvent(uint dwDesiredAccess, bool bInheritHandle, string lpName);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetExitCodeProcess(IntPtr hProcess, out uint lpExitCode);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

        [DllImport("user32.dll")]
        private static extern IntPtr GetShellWindow();

        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out int lpdwProcessId);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool CreateProcess(
            string lpApplicationName,
            string lpCommandLine,
            IntPtr lpProcessAttributes,
            IntPtr lpThreadAttributes,
            bool bInheritHandles,
            uint dwCreationFlags,
            IntPtr lpEnvironment,
            string lpCurrentDirectory,
            ref STARTUPINFOEX lpStartupInfo,
            out PROCESS_INFORMATION lpProcessInformation);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool InitializeProcThreadAttributeList(
            IntPtr lpAttributeList,
            int dwAttributeCount,
            int dwFlags,
            ref IntPtr lpSize);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool UpdateProcThreadAttribute(
            IntPtr lpAttributeList,
            uint dwFlags,
            IntPtr attribute,
            ref IntPtr lpValue,
            IntPtr cbSize,
            IntPtr lpPreviousValue,
            IntPtr lpReturnSize);

        [DllImport("kernel32.dll")]
        private static extern void DeleteProcThreadAttributeList(IntPtr lpAttributeList);

        /// <summary>Resolves the paths to the server and helper executables once per fixture.</summary>
        [OneTimeSetUp]
        public void OneTimeSetUp()
        {
            string binDir = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);

            this.serverPath = TestSetup.Parameters.WinGetServerPath
                ?? Path.GetFullPath(Path.Join(binDir, @"..\WinGetServer\WindowsPackageManagerServer.exe"));

            this.helperPath = TestSetup.Parameters.RpcTestHelperPath
                ?? Path.GetFullPath(Path.Join(binDir, @"..\WinGetRpcTestHelper\WinGetRpcTestHelper.exe"));
        }

        /// <summary>
        /// Verifies that a medium-integrity process cannot open the server-start event with
        /// EVENT_MODIFY_STATE, which is the access it would need to signal the event early and
        /// defeat the client's wait for the server to become ready. Validates both the per-user
        /// event name and its mandatory label.
        /// </summary>
        [Test]
        public void MediumIntegrityClient_CannotSignalEvent()
        {
            string sid = GetCurrentUserSID();
            Process server = this.StartServer(sid);
            try
            {
                string eventName = "WinGetServerStartEvent_" + sid;
                int rc = this.RunHelperAtMediumIntegrity(
                    $"\"{this.helperPath}\" --mode event-signal --event-name {eventName}");

                string message = rc == 1
                    ? "Medium-integrity process opened the event with EVENT_MODIFY_STATE - per-user name or mandatory label is missing."
                    : $"Helper inconclusive (exit {rc}).";
                Assert.That(rc, Is.EqualTo(0), message);
            }
            finally
            {
                KillProcess(server);
            }
        }

        /// <summary>
        /// End-to-end: a medium-integrity client cannot complete an RPC call to the server.
        /// Two independent layers should deny it: the security descriptor on the RPC endpoint,
        /// which the OS enforces when the client resolves the endpoint in order to connect,
        /// and the interface security descriptor, which the RPC runtime enforces by
        /// impersonating the caller and running an access check.
        /// </summary>
        [Test]
        public void MediumIntegrityClient_CannotConnectViaRpc()
        {
            string sid = GetCurrentUserSID();
            Process server = this.StartServer(sid);
            try
            {
                int rc = this.RunHelperAtMediumIntegrity(
                    $"\"{this.helperPath}\" --mode rpc-connect");

                string message = rc == 0
                    ? "Medium-integrity client successfully reached the server via RPC - neither the endpoint security descriptor nor the interface security descriptor is blocking lower-integrity callers. "
                      + "Check that both mandatory labels use NRNWNX; a bare NW still leaves access granted through GENERIC_ALL."
                    : $"Unexpected error 0x{rc:X8} (expected ERROR_ACCESS_DENIED / 0x{RpcErrorAccessDeniedHResult:X8}).";
                Assert.That(rc, Is.EqualTo(RpcErrorAccessDeniedHResult), message);
            }
            finally
            {
                KillProcess(server);
            }
        }

        /// <summary>
        /// Positive case: confirms that an elevated (high-integrity) client can successfully
        /// reach the server via authenticated RPC. Guards against over-restricting legitimate callers.
        /// </summary>
        [Test]
        public void ElevatedClient_CanConnectViaRpc()
        {
            string sid = GetCurrentUserSID();
            Process server = this.StartServer(sid);
            try
            {
                int rc = this.RunHelper($"--mode rpc-connect");

                string message = rc != 0
                    ? $"Elevated client was rejected by the server (0x{rc:X8}) - the security configuration is blocking legitimate elevated callers."
                    : string.Empty;
                Assert.That(rc, Is.EqualTo(0), message);
            }
            finally
            {
                KillProcess(server);
            }
        }

        /// <summary>
        /// Verifies that an elevated client rejects a medium-integrity server process.
        /// The rpc-connect mode calls <c>WinGetServerManualActivation_CreateInstance</c>
        /// (the production client function) which, after the production fix, will check
        /// the server process integrity inside <c>InitializeRpcBinding</c> and return
        /// <c>ERROR_ACCESS_DENIED</c> when the server is below high integrity.
        /// </summary>
        [Test]
        public void ElevatedClient_RejectsMediumIntegrityServer()
        {
            string sid = GetCurrentUserSID();

            Process medServer = this.StartHelperAtMediumIntegrity(
                $"\"{this.serverPath}\" --manualActivation");
            try
            {
                string readyEventName = "WinGetServerStartEvent_" + sid;
                this.WaitForServerReadyEvent(readyEventName, medServer);

                int rc = this.RunHelper($"--mode rpc-connect");

                string message = rc == 0
                    ? "Elevated client connected to a medium-integrity server - client-side process integrity check is missing."
                    : $"Unexpected error 0x{rc:X8} (expected ERROR_ACCESS_DENIED / 0x{RpcErrorAccessDeniedHResult:X8}).";
                Assert.That(rc, Is.EqualTo(RpcErrorAccessDeniedHResult), message);
            }
            finally
            {
                KillProcess(medServer);
            }
        }

        /// <summary>
        /// Verifies that a medium-integrity process cannot open the single-instance server mutex
        /// with SYNCHRONIZE, which is the access it would need to acquire the mutex and hold it
        /// so that the elevated server exits with ERROR_SERVICE_ALREADY_RUNNING instead of
        /// starting. Validates both the per-user mutex name and its mandatory label.
        /// </summary>
        [Test]
        public void MediumIntegrityClient_CannotAcquireServerMutex()
        {
            string sid = GetCurrentUserSID();
            Process server = this.StartServer(sid);
            try
            {
                string mutexName = "WinGetServerMutex_" + sid;
                int rc = this.RunHelperAtMediumIntegrity(
                    $"\"{this.helperPath}\" --mode mutex-open --mutex-name {mutexName}");

                string message = rc == 1
                    ? "Medium-integrity process opened the server mutex with SYNCHRONIZE and could hold it to block the elevated server from starting - per-user name or mandatory label is missing. "
                      + "Note that the mutex label must be NRNWNX; unlike the event, a bare NW leaves the wait access granted through the generic execute right."
                    : $"Helper inconclusive (exit {rc}).";
                Assert.That(rc, Is.EqualTo(0), message);
            }
            finally
            {
                KillProcess(server);
            }
        }

        private static string GetCurrentUserSID()
        {
            return WindowsIdentity.GetCurrent().User?.Value
                ?? throw new InvalidOperationException("Cannot determine current user SID");
        }

        private static void KillProcess(Process p)
        {
            if (p == null)
            {
                return;
            }

            try
            {
                if (!p.HasExited)
                {
                    p.Kill();
                    p.WaitForExit(5000);
                }
            }
            catch (Exception)
            {
                // Best-effort cleanup; do not mask the test result.
            }
            finally
            {
                p.Dispose();
            }
        }

        /// <summary>Starts the real WinGetServer in --manualActivation mode and waits for it to signal
        /// its per-user ready event.</summary>
        private Process StartServer(string sid)
        {
            var proc = Process.Start(new ProcessStartInfo
            {
                FileName = this.serverPath,
                Arguments = "--manualActivation",
                UseShellExecute = false,
            }) ?? throw new InvalidOperationException("Failed to start WinGetServer");

            string readyEventName = "WinGetServerStartEvent_" + sid;
            this.WaitForServerReadyEvent(readyEventName, proc);
            return proc;
        }

        /// <summary>Polls for <paramref name="eventName"/> to appear and waits for it to be signaled.</summary>
        /// <param name="eventName">The name of the manual-reset event the server signals when ready.</param>
        /// <param name="serverProcess">
        /// Optional server process to monitor. If the process exits before the event is found
        /// the exception message includes its exit code to aid diagnosis.
        /// </param>
        private void WaitForServerReadyEvent(string eventName, Process serverProcess = null)
        {
            IntPtr hEvent = IntPtr.Zero;
            const int PollMs = 200;
            const int MaxWaitMs = 15000;
            int elapsed = 0;

            while (hEvent == IntPtr.Zero && elapsed < MaxWaitMs)
            {
                if (serverProcess != null && serverProcess.HasExited)
                {
                    string exitInfo = this.TryGetProcessExitCode(serverProcess.Id, out uint code)
                        ? $"0x{code:X8} ({code})"
                        : "unknown";
                    throw new InvalidOperationException(
                        $"Server process exited (exit code {exitInfo}) before signaling ready event '{eventName}'.");
                }

                hEvent = OpenEvent(Synchronize, false, eventName);
                if (hEvent == IntPtr.Zero)
                {
                    Thread.Sleep(PollMs);
                    elapsed += PollMs;
                }
            }

            if (hEvent == IntPtr.Zero)
            {
                string processInfo = string.Empty;
                if (serverProcess != null)
                {
                    processInfo = serverProcess.HasExited
                        ? (this.TryGetProcessExitCode(serverProcess.Id, out uint code)
                            ? $" Server exited with code 0x{code:X8} ({code})."
                            : " Server has exited.")
                        : " Server is still running.";
                }

                throw new InvalidOperationException(
                    $"Server-ready event '{eventName}' not found after {MaxWaitMs}ms.{processInfo}");
            }

            try
            {
                uint result = WaitForSingleObject(hEvent, 10000);
                if (result != WaitObject0)
                {
                    throw new InvalidOperationException(
                        $"Timed out waiting for server-ready event '{eventName}' (0x{result:X8}).");
                }
            }
            finally
            {
                CloseHandle(hEvent);
            }
        }

        /// <summary>
        /// Opens the given process by ID and reads its exit code.
        /// Returns false if the handle cannot be opened or the process has not yet exited.
        /// </summary>
        private bool TryGetProcessExitCode(int pid, out uint exitCode)
        {
            exitCode = 0;
            IntPtr hProcess = OpenProcess(ProcessQueryLimitedInformation, false, pid);
            if (hProcess == IntPtr.Zero)
            {
                return false;
            }

            try
            {
                return GetExitCodeProcess(hProcess, out exitCode) && exitCode != 259 /* STILL_ACTIVE */;
            }
            finally
            {
                CloseHandle(hProcess);
            }
        }

        /// <summary>Runs the helper at the current (elevated) integrity level and returns its exit code.</summary>
        private int RunHelper(string args)
        {
            using var p = Process.Start(new ProcessStartInfo
            {
                FileName = this.helperPath,
                Arguments = args,
                UseShellExecute = false,
            }) ?? throw new InvalidOperationException("Failed to start helper");

            bool done = p.WaitForExit(60000);
            Assert.That(done, Is.True, "Helper (elevated) did not exit within 60 seconds.");
            return p.ExitCode;
        }

        /// <summary>
        /// Runs the given command line as a new process at medium mandatory integrity level and
        /// returns its exit code.
        /// </summary>
        private int RunHelperAtMediumIntegrity(string commandLine)
        {
            return this.RunProcessAtMediumIntegrity(commandLine, waitForExit: true).exitCode;
        }

        /// <summary>Starts the given command line at medium integrity without waiting for it to exit.</summary>
        private Process StartHelperAtMediumIntegrity(string commandLine)
        {
            return this.RunProcessAtMediumIntegrity(commandLine, waitForExit: false).process;
        }

        /// <summary>
        /// Core helper that spawns a process using the shell window's process as parent,
        /// so the child inherits Explorer's unelevated token. Either waits for exit
        /// (returning the exit code) or returns the running Process object.
        /// </summary>
        private (Process process, int exitCode) RunProcessAtMediumIntegrity(
            string commandLine,
            bool waitForExit)
        {
            // Find the shell (Explorer) window and open it with PROCESS_CREATE_PROCESS.
            // The new process inherits Explorer's unelevated token via the parent attribute.
            IntPtr shellWnd = GetShellWindow();
            if (shellWnd == IntPtr.Zero)
            {
                throw new InvalidOperationException("GetShellWindow returned null.");
            }

            GetWindowThreadProcessId(shellWnd, out int shellPid);
            IntPtr hShell = OpenProcess(ProcessCreateProcess, false, shellPid);
            if (hShell == IntPtr.Zero)
            {
                throw new Win32Exception(Marshal.GetLastWin32Error(), "OpenProcess(shell)");
            }

            try
            {
                // Allocate a PROC_THREAD_ATTRIBUTE_LIST for one attribute.
                IntPtr attrListSize = IntPtr.Zero;
                InitializeProcThreadAttributeList(IntPtr.Zero, 1, 0, ref attrListSize);
                IntPtr pAttrList = Marshal.AllocHGlobal(attrListSize);
                try
                {
                    if (!InitializeProcThreadAttributeList(pAttrList, 1, 0, ref attrListSize))
                    {
                        throw new Win32Exception(Marshal.GetLastWin32Error(), "InitializeProcThreadAttributeList");
                    }

                    try
                    {
                        // Tell CreateProcess to use the shell process as the parent.
                        IntPtr hShellCopy = hShell;
                        if (!UpdateProcThreadAttribute(
                                pAttrList,
                                0,
                                ProcThreadAttributeParentProcess,
                                ref hShellCopy,
                                new IntPtr(IntPtr.Size),
                                IntPtr.Zero,
                                IntPtr.Zero))
                        {
                            throw new Win32Exception(Marshal.GetLastWin32Error(), "UpdateProcThreadAttribute");
                        }

                        var siex = new STARTUPINFOEX
                        {
                            StartupInfo = new STARTUPINFO { cb = Marshal.SizeOf<STARTUPINFOEX>() },
                            lpAttributeList = pAttrList,
                        };

                        if (!CreateProcess(
                                null,
                                commandLine,
                                IntPtr.Zero,
                                IntPtr.Zero,
                                false,
                                CreateNoWindow | ExtendedStartupInfoPresent,
                                IntPtr.Zero,
                                Directory.GetCurrentDirectory(),
                                ref siex,
                                out PROCESS_INFORMATION pi))
                        {
                            throw new Win32Exception(Marshal.GetLastWin32Error(), "CreateProcess");
                        }

                        CloseHandle(pi.hThread);

                        if (waitForExit)
                        {
                            try
                            {
                                uint waitResult = WaitForSingleObject(pi.hProcess, 60000);
                                Assert.That(
                                    waitResult,
                                    Is.EqualTo(WaitObject0),
                                    "Medium-integrity helper did not exit within 60 seconds.");

                                if (!GetExitCodeProcess(pi.hProcess, out uint exitCode))
                                {
                                    throw new Win32Exception(
                                        Marshal.GetLastWin32Error(), "GetExitCodeProcess");
                                }

                                return (null, (int)exitCode);
                            }
                            finally
                            {
                                CloseHandle(pi.hProcess);
                            }
                        }
                        else
                        {
                            Process childProcess;
                            try
                            {
                                childProcess = Process.GetProcessById(pi.dwProcessId);
                            }
                            catch
                            {
                                CloseHandle(pi.hProcess);
                                throw;
                            }

                            CloseHandle(pi.hProcess);
                            return (childProcess, 0);
                        }
                    }
                    finally
                    {
                        DeleteProcThreadAttributeList(pAttrList);
                    }
                }
                finally
                {
                    Marshal.FreeHGlobal(pAttrList);
                }
            }
            finally
            {
                CloseHandle(hShell);
            }
        }

        // Nested types (P/Invoke structs) come last per StyleCop SA1201.
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct STARTUPINFO
        {
            public int cb;
            public string lpReserved;
            public string lpDesktop;
            public string lpTitle;
            public int dwX;
            public int dwY;
            public int dwXSize;
            public int dwYSize;
            public int dwXCountChars;
            public int dwYCountChars;
            public int dwFillAttribute;
            public int dwFlags;
            public short wShowWindow;
            public short cbReserved2;
            public IntPtr lpReserved2;
            public IntPtr hStdInput;
            public IntPtr hStdOutput;
            public IntPtr hStdError;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct STARTUPINFOEX
        {
            public STARTUPINFO StartupInfo;
            public IntPtr lpAttributeList;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct PROCESS_INFORMATION
        {
            public IntPtr hProcess;
            public IntPtr hThread;
            public int dwProcessId;
            public int dwThreadId;
        }
    }
}
