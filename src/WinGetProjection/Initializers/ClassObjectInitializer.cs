namespace WinGetProjection
{
    using System;
    using System.Runtime.InteropServices;
    using WinRT;

    public class ClassObjectInitializer : IInstanceInitializer
    {
        private delegate int DllGetClassObject(
            ref Guid clsid,
            ref Guid iid,
            [Out, MarshalAs(UnmanagedType.Interface)] out IClassFactory ppv);

        private IntPtr moduleHandle;
        private DllGetClassObject dllGetClassObject;

        public unsafe ClassObjectInitializer(string fileName)
        {
            moduleHandle = ComUtils.LoadModule(fileName);
            IntPtr dllGetClassObjectPtr = ComUtils.GetProcAddress(moduleHandle, nameof(DllGetClassObject));
            dllGetClassObject = (DllGetClassObject)Marshal.GetDelegateForFunctionPointer(dllGetClassObjectPtr, typeof(DllGetClassObject));
        }

        public unsafe T CreateInstance<T>() where T : new()
        {
            var clsid = ComClsids.GetCLSID<T>(inProc: true);
            var iid = ComClsids.GetIID<T>();
            var classFactoryIID = typeof(IClassFactory).GUID;

            var hr = dllGetClassObject(ref clsid, ref classFactoryIID, out IClassFactory classFactory);
            Marshal.ThrowExceptionForHR(hr);

            hr = classFactory.CreateInstance(IntPtr.Zero, iid, out IntPtr instancePtr);
            Marshal.ThrowExceptionForHR(hr);

            return MarshalGeneric<T>.FromAbi(instancePtr);
        }

        // Enables a class of objects to be created.
        // https://docs.microsoft.com/windows/win32/api/unknwn/nn-unknwn-iclassfactory
        [ComImport]
        [ComVisible(false)]
        [Guid("00000001-0000-0000-C000-000000000046")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        private interface IClassFactory
        {
            [PreserveSig]
            public int CreateInstance(IntPtr pUnkOuter, ref Guid riid, out IntPtr ppvObject);

            [PreserveSig]
            public int LockServer(bool fLock);
        }
    }
}
