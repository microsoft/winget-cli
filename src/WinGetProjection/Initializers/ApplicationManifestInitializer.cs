namespace WinGetProjection
{
    using WinRT;

    // Registration-Free COM Interop using provided application manifest
    // deployed side-by-side of the coresponding assemblies
    public class ApplicationManifestInitializer : IInstanceInitializer
    {
        public T CreateInstance<T>() where T : new()
        {
            var clsid = ComClsids.GetCLSID<T>(inProc: true);
            var iid = ComClsids.GetIID<T>();

            var instanceInPtr = ComUtils.CoCreateInstanceInProcServer(clsid, iid);
            return MarshalGeneric<T>.FromAbi(instanceInPtr);
        }
    }
}
