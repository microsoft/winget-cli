namespace Microsoft.Management.Deployment.Projection
{
    // Registration-Free COM Interop using provided application manifest
    // deployed side-by-side of the coresponding assemblies
    public class ApplicationManifestInitializer : IInstanceInitializer
    {
        /// <summary>
        /// In-process context.
        /// </summary>
        public ClsidContext Context => ClsidContext.InProc;

        /// <summary>
        /// Create instance of the provided type.
        /// </summary>
        /// <typeparam name="T">Projected class type.</typeparam>
        /// <returns>Instance of the provided type.</returns>
        public T CreateInstance<T>()
            where T : new()
        {
            var clsid = Projections.GetClsid<T>(Context);
            var iid = Projections.GetIid<T>();

            var instanceInPtr = ComUtils.CoCreateInstanceInProcServer(clsid, iid);
            return WinRT.MarshalGeneric<T>.FromAbi(instanceInPtr);
        }
    }
}
