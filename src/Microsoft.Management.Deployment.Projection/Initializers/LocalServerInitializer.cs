namespace Microsoft.Management.Deployment.Projection
{
    using WinRT;

    // Out-of-process COM server (WindowsPackageManagerServer.exe)
    public class LocalServerInitializer : IInstanceInitializer
    {
        /// <summary>
        /// Out-of-process context.
        /// </summary>
        public ClsidContext Context { get; } = ClsidContext.OutOfProc;

        /// <summary>
        /// Allow lower trust registration.
        /// This is useful when running COM client with admin privileges. (e.g. E2E tests)
        /// </summary>
        public bool AllowLowerTrustRegistration { init; get; }

        /// <summary>
        /// Create a local server initializer
        /// </summary>
        /// <param name="useDevClsids">Use development Clsids.</param>
        public LocalServerInitializer(bool useDevClsids = false)
        {
            if (useDevClsids)
            {
                Context = ClsidContext.OutOfProcDev;
            }
        }

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

            var instanceInPtr = ComUtils.CoCreateInstanceLocalServer(clsid, iid, AllowLowerTrustRegistration);
            return MarshalGeneric<T>.FromAbi(instanceInPtr);
        }
    }
}
