// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.Management.Deployment.Projection
{
    using Microsoft.Management.Deployment.Projection.Initializers;

    /// <summary>
    /// Activation factory instance initializer requires that:
    /// - DllGetActivationFactory is exported
    ///   More details: https://github.com/microsoft/CsWinRT/blob/master/docs/hosting.md#exports
    /// - Host dll file name should be Microsoft.Management.Deployment.dll or a child namespace
    ///   to match the projected classes (e.g. PackageManager) namespace
    ///   More details: https://docs.microsoft.com/en-us/windows/apps/develop/platform/csharp-winrt/#winrt-type-activation
    /// </summary>
    public class ActivationFactoryInstanceInitializer : PolicyEnforcedInstanceInitializer
    {
        /// <summary>
        /// In-process context
        /// </summary>
        public override ClsidContext Context => ClsidContext.InProc;

        /// <summary>
        /// Calls default projected class constructor implemented by CsWinRT.
        /// Default constructor uses DllGetActivationFactory to create an object
        /// based on the full name of the projected class.
        /// </summary>
        /// <typeparam name="T">Projected class type</typeparam>
        /// <returns>Instance of the provided type.</returns>
        protected override T CreateInstanceInternal<T>() => new();
    }
}
