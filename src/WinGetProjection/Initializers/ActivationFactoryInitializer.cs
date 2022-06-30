
namespace WinGetProjection
{
    // Activation factory initializer requires that:
    // - DllGetActivationFactory is exported
    //   More details: https://github.com/microsoft/CsWinRT/blob/master/docs/hosting.md#exports
    // - Host dll file name should be Microsoft.Management.Deployment.dll or a child namespace
    //   to match the projected classes (e.g. PackageManager) namespace
    //   More details: https://docs.microsoft.com/en-us/windows/apps/develop/platform/csharp-winrt/#winrt-type-activation
    public class ActivationFactoryInitializer : IInstanceInitializer
    {
        public T CreateInstance<T>() where T : new()
        {
            return new();
        }
    }
}
