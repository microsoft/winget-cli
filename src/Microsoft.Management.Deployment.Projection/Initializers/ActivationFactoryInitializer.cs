namespace Microsoft.Management.Deployment.Projection
{
    /// <summary>
    /// Activation factory initializer requires that:
    /// - DllGetActivationFactory is exported
    ///   More details: https://github.com/microsoft/CsWinRT/blob/master/docs/hosting.md#exports
    /// - Host dll file name should be Microsoft.Management.Deployment.dll or a child namespace
    ///   to match the projected classes (e.g. PackageManager) namespace
    ///   More details: https://docs.microsoft.com/en-us/windows/apps/develop/platform/csharp-winrt/#winrt-type-activation
    /// </summary>
    public class ActivationFactoryInitializer : IInstanceInitializer
    {
        /// <summary>
        /// In-process context
        /// </summary>
        public ClsidContext Context => ClsidContext.InProc;

        /// <summary>
        /// Calls default projected class constructor implemented by CsWinRT.
        /// </summary>
        /// <typeparam name="T">Projected class type</typeparam>
        /// <returns>Instance of the provided type.</returns>
        public T CreateInstance<T>() where T : new() => new();
    }
}
