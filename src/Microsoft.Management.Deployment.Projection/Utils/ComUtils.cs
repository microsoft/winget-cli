// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.Management.Deployment.Projection
{
    using System;
    using System.Runtime.InteropServices;
    using WinRT;

    internal static class ComUtils
    {
        /// <summary>
        /// CLSCTX enumeration
        /// https://docs.microsoft.com/en-us/windows/win32/api/wtypesbase/ne-wtypesbase-clsctx
        /// </summary>
        private enum CLSCTX : uint
        {
            CLSCTX_INPROC_SERVER = 0x1,
            CLSCTX_LOCAL_SERVER = 0x4,
            CLSCTX_ALLOW_LOWER_TRUST_REGISTRATION = 0x4000000
        }

        /// <summary>
        /// CoCreateInstance function
        /// https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-cocreateinstance
        /// </summary>
        /// <param name="clsid">CLSID</param>
        /// <param name="clsContext">CLSCTX</param>
        /// <param name="iid">IID</param>
        /// <returns>Interface pointer, or throw an exception if HRESULT was not successful.</returns>
        private static unsafe IntPtr CoCreateInstance(Guid clsid, CLSCTX clsContext, Guid iid)
        {
            IntPtr instanceIntPtr;
            int hr = Platform.CoCreateInstance(ref clsid, IntPtr.Zero, (uint)clsContext, ref iid, &instanceIntPtr);
            Marshal.ThrowExceptionForHR(hr);
            return instanceIntPtr;
        }

        /// <summary>
        /// CoCreateInstance with an out-of-process context.
        /// </summary>
        /// <param name="clsid">CLSID</param>
        /// <param name="iid">CLSCTX</param>
        /// <param name="allowLowerTrustRegistration">Allow lower trust registration</param>
        /// <returns><see cref="CoCreateInstance"/> ></returns>
        public static IntPtr CoCreateInstanceLocalServer(Guid clsid, Guid iid, bool allowLowerTrustRegistration = false)
        {
            CLSCTX clsctx = CLSCTX.CLSCTX_LOCAL_SERVER;
            if (allowLowerTrustRegistration)
            {
                clsctx |= CLSCTX.CLSCTX_ALLOW_LOWER_TRUST_REGISTRATION;
            }

            return CoCreateInstance(clsid, clsctx, iid);
        }
    }
}
