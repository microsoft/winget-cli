// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System.Runtime.InteropServices;
using System.Threading;

namespace ConfigurationRemotingServer
{
    internal class EnvironmentChangeListener
    {
        // Window message constants
        private const int WM_SETTINGCHANGE = 0x001A;
        private const int WM_QUIT = 0x0012;

        // User32.dll imports
        [DllImport("user32.dll")]
        private static extern IntPtr CreateWindowEx(
            uint dwExStyle, string lpClassName, string lpWindowName, uint dwStyle,
            int x, int y, int nWidth, int nHeight, IntPtr hWndParent, IntPtr hMenu,
            IntPtr hInstance, IntPtr lpParam);

        [DllImport("user32.dll")]
        private static extern bool DestroyWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        private static extern IntPtr DefWindowProc(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll")]
        private static extern bool GetMessage(out MSG lpMsg, IntPtr hWnd, uint wMsgFilterMin, uint wMsgFilterMax);

        [DllImport("user32.dll")]
        private static extern bool PostMessageW(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll")]
        private static extern bool TranslateMessage(ref MSG lpMsg);

        [DllImport("user32.dll")]
        private static extern IntPtr DispatchMessage(ref MSG lpMsg);

        [DllImport("user32.dll")]
        private static extern ushort RegisterClass(ref WNDCLASS lpWndClass);

        [DllImport("kernel32.dll")]
        private static extern IntPtr GetModuleHandle(string? lpModuleName);

        // Windows message structure
        [StructLayout(LayoutKind.Sequential)]
        public struct MSG
        {
            public IntPtr hwnd;
            public uint message;
            public IntPtr wParam;
            public IntPtr lParam;
            public uint time;
            public POINT pt;
            public uint lPrivate;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct POINT
        {
            public int x;
            public int y;
        }

        // Window class structure
        [StructLayout(LayoutKind.Sequential)]
        public struct WNDCLASS
        {
            public uint style;
            public IntPtr lpfnWndProc;
            public int cbClsExtra;
            public int cbWndExtra;
            public IntPtr hInstance;
            public IntPtr hIcon;
            public IntPtr hCursor;
            public IntPtr hBackground;
            [MarshalAs(UnmanagedType.LPStr)]
            public string? lpszMenuName;
            [MarshalAs(UnmanagedType.LPStr)]
            public string lpszClassName;
        }

        // Window procedure delegate
        private delegate IntPtr WndProcDelegate(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);
        // Keep a reference to the delegate to prevent it from being garbage collected
        private static WndProcDelegate? _wndProcDelegate;

        private IntPtr _hwnd;
        private Thread _thread;

        public delegate void EnvironmentChangedEventHandler();
        public static event EnvironmentChangedEventHandler? EnvironmentChanged;

        public EnvironmentChangeListener()
        {
            _thread = new Thread(MessageLoop);
            _thread.Start();
        }

        public void Stop()
        {
            if (PostMessageW(_hwnd, WM_QUIT, IntPtr.Zero, IntPtr.Zero))
            {
                _thread.Join();
            }
        }

        private void MessageLoop()
        {
            // Register window class
            string className = "EnvironmentMonitorClass";
            _wndProcDelegate = WndProc;

            WNDCLASS wndClass = new WNDCLASS
            {
                style = 0,
                lpfnWndProc = Marshal.GetFunctionPointerForDelegate(_wndProcDelegate),
                cbClsExtra = 0,
                cbWndExtra = 0,
                hInstance = GetModuleHandle(null),
                hIcon = IntPtr.Zero,
                hCursor = IntPtr.Zero,
                hBackground = IntPtr.Zero,
                lpszMenuName = "",
                lpszClassName = className
            };

            RegisterClass(ref wndClass);

            // Create hidden window
            const uint WS_OVERLAPPED = 0x00000000;
            const uint WS_EX_TOOL_WINDOW = 0x00000080;

            _hwnd = CreateWindowEx(
                WS_EX_TOOL_WINDOW,
                className,
                "Environment Monitor",
                WS_OVERLAPPED,
                0, 0, 0, 0,
                IntPtr.Zero, IntPtr.Zero,
                GetModuleHandle(null), IntPtr.Zero);

            if (_hwnd == IntPtr.Zero)
            {
                return;
            }

            MSG msg;
            while (GetMessage(out msg, IntPtr.Zero, 0, 0))
            {
                TranslateMessage(ref msg);
                DispatchMessage(ref msg);
            }

            // Clean up
            DestroyWindow(_hwnd);
        }

        private static IntPtr WndProc(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam)
        {
            switch (msg)
            {
                case WM_SETTINGCHANGE:
                    // Check if this is an environment variable change notification
                    string? msgInfo = Marshal.PtrToStringAnsi(lParam);
                    if (msgInfo == "Environment")
                    {
                        if (EnvironmentChanged != null)
                        {
                            EnvironmentChanged.Invoke();
                        }
                    }
                    break;
            }

            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
    }
}
