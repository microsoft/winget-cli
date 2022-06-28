
namespace WinGetProjection
{
    using System;
    using System.Runtime.InteropServices;

    // Activation factory initializer requires that:
    // - DllGetActivationFactory is implemented
    // - Host dll file name should be Microsoft.Management.Deployment.dll or a child namespace
    //   to match the projected classes (e.g. PackageManager) namespace
    public class ActivationFactoryInitializer : IInstanceInitializer
    {

        private class Prelink
        {
            [DllImport("Microsoft.Management.Deployment.dll")]
            public static unsafe extern int DllGetActivationFactory(IntPtr activatableClassId, IntPtr* factory);
        }

        public ActivationFactoryInitializer()
        {
            // Ensure that required dll is loaded and function is implemented
            Marshal.PrelinkAll(typeof(Prelink));
        }

        public T CreateInstance<T>() where T : new()
        {
            return new();
        }
    }
}
