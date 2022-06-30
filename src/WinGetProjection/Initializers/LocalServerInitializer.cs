namespace WinGetProjection
{
    using WinRT;

    // Out-of-process COM server (WinGetServer\WindowsPackageManagerServer.exe)
    public class LocalServerInitializer : IInstanceInitializer
    {
        public T CreateInstance<T>() where T : new()
        {
            var clsid = ComClsids.GetCLSID<T>(inProc: false);
            var iid = ComClsids.GetIID<T>();

            var instanceInPtr = ComUtils.CoCreateInstanceLocalServer(clsid, iid);
            return MarshalGeneric<T>.FromAbi(instanceInPtr);
        }
    }
}
