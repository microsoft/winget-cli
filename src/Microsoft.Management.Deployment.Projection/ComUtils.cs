namespace Microsoft.Management.Deployment.Projection
{
    using System;
    using System.Runtime.InteropServices;
    using WinRT;

    public static class ComUtils
    {
        private enum CLSCTX : uint
        {
            CLSCTX_INPROC_SERVER = 0x1,
            CLSCTX_LOCAL_SERVER = 0x4,
            CLSCTX_ALLOW_LOWER_TRUST_REGISTRATION = 0x4000000
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

        public static IntPtr CoCreateInstanceLocalServer(Guid clsid, Guid iid, bool allowLowerTrustReg = false)
        {
            CLSCTX clsctx = CLSCTX.CLSCTX_LOCAL_SERVER;
            if (allowLowerTrustReg)
            {
                clsctx |= CLSCTX.CLSCTX_ALLOW_LOWER_TRUST_REGISTRATION;
            }

            return CoCreateInstance(clsid, clsctx, iid);
        }
    }
}
