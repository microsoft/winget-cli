namespace WinGetProjection
{
    using System;
    using System.Runtime.InteropServices;
    using WinRT;

    public static class ComUtils
    {
        private enum CLSCTX : uint
        {
            CLSCTX_INPROC_SERVER = 0x1,
            CLSCTX_LOCAL_SERVER = 0x4
        }

        private static unsafe IntPtr CoCreateInstance(Guid clsid, CLSCTX clsContext, Guid iid)
        {
            IntPtr instanceIntPtr;
            int hr = Platform.CoCreateInstance(ref clsid, IntPtr.Zero, (uint)clsContext, ref iid, &instanceIntPtr);
            Marshal.ThrowExceptionForHR(hr);
            return instanceIntPtr;
        }

        public static IntPtr CoCreateInstanceInProcServer(Guid clsid, Guid iid)
        {
            return CoCreateInstance(clsid, CLSCTX.CLSCTX_INPROC_SERVER, iid);
        }

        public static IntPtr CoCreateInstanceLocalServer(Guid clsid, Guid iid)
        {
            return CoCreateInstance(clsid, CLSCTX.CLSCTX_LOCAL_SERVER, iid);
        }

        public static IntPtr LoadModule(string fileName)
        {
            var moduleHandle = Platform.LoadLibraryExW(fileName, IntPtr.Zero, /* LOAD_WITH_ALTERED_SEARCH_PATH */ 8);
            if (moduleHandle == IntPtr.Zero)
            {
                throw new ArgumentException($"Module not found: {fileName}");
            }
            return moduleHandle;
        }

        public static unsafe void* GetProcAddress(IntPtr moduleHandle, string procName)
        {
            if (moduleHandle == IntPtr.Zero)
            {
                throw new ArgumentException("Module handle cannot be empty");
            }

            return Platform.GetProcAddress(moduleHandle, procName);
        }
    }
}
