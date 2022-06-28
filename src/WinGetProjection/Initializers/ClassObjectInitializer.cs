namespace WinGetProjection
{
    using System;
    using System.Runtime.InteropServices;
    using WinRT;

    public class ClassObjectInitializer : IInstanceInitializer
    {
        private IntPtr moduleHandle;
        private unsafe delegate* unmanaged[Stdcall]<ref Guid, ref Guid, void*, int> dllGetClassObject;

        public unsafe ClassObjectInitializer(string fileName)
        {
            moduleHandle = ComUtils.LoadModule(fileName);
            dllGetClassObject = (delegate* unmanaged[Stdcall]<ref Guid, ref Guid, void*, int>)ComUtils.GetProcAddress(moduleHandle, "DllGetClassObject");
        }

        public unsafe T CreateInstance<T>() where T : new()
        {
            var clsid = ComClsids.GetCLSID<T>(inProc: true);
            var iid = ComClsids.GetIID<T>();

            IntPtr instanceIntPtr;
            var hr = dllGetClassObject(ref clsid, ref iid, &instanceIntPtr);
            Marshal.ThrowExceptionForHR(hr);
            return MarshalGeneric<T>.FromAbi(instanceIntPtr);
        }
    }
}
