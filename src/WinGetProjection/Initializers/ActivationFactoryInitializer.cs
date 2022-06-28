
namespace WinGetProjection
{
    using System;
    using System.Runtime.InteropServices;

    // Activation factory initializer requires that:
    // - DllGetActivationFactory is implemented
    // - Host dll file name should be Microsoft.Management.Deployment.dll or a child namespace
    //   to match the projected classes (e.g. PackageManager) namespace
    // Note: The implementation for using DllGetActivationFactory is handled by CsWinRT.
    //       For more details, build the solution then inspect the generated 'WinRT.cs' file.
    public class ActivationFactoryInitializer : IInstanceInitializer
    {
        public ActivationFactoryInitializer()
        {
            Validate();
        }

        public T CreateInstance<T>() where T : new()
        {
            return new();
        }

        public void Validate()
        {
            // Ensure that required dll is loaded and function is implemented
            Marshal.PrelinkAll(typeof(RequiredImports));
        }

        /// <summary>
        /// Methods in this class are required for this initializer to work
        /// <seealso cref="Validate"/>
        /// </summary>
        private class RequiredImports
        {
            [DllImport("Microsoft.Management.Deployment.dll")]
            public static unsafe extern int DllGetActivationFactory(IntPtr activatableClassId, IntPtr* factory);
        }
    }
}
