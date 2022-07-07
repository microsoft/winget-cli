namespace Microsoft.Management.Deployment.Projection
{
    using WinRT;

    /// <summary>
    /// Registration-Free COM Interop using provided application manifest
    /// deployed side-by-side of the coresponding assemblies. Manifest should contain:
    /// <code>
    /// &lt;dependentAssembly&gt;
    ///   &lt;assemblyIdentity name="Microsoft.Management.Deployment.InProc.dll" version="0.0.0.0" /&gt;
    /// &lt;/dependentAssembly&gt;
    /// </code>
    /// </summary>
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
            var clsid = ClassesDefinition.GetClsid<T>(Context);
            var iid = ClassesDefinition.GetIid<T>();

            var instanceInPtr = ComUtils.CoCreateInstanceInProcServer(clsid, iid);
            return MarshalGeneric<T>.FromAbi(instanceInPtr);
        }
    }
}
