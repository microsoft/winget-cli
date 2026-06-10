// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.Management.Deployment.Projection
{
    using Microsoft.Management.Deployment.Projection.Initializers;
    using WinRT;

    // Out-of-process COM instance initializer.
    public class LocalServerInstanceInitializer : PolicyEnforcedInstanceInitializer
    {
        /// <summary>
        /// Out-of-process context.
        /// </summary>
        public override ClsidContext Context => UseDevClsids ? ClsidContext.OutOfProcDev : ClsidContext.OutOfProc;

        /// <summary>
        /// Allow lower trust registration.
        /// This is useful when running COM client with admin privileges. (e.g. E2E tests)
        /// </summary>
        public bool AllowLowerTrustRegistration { init; get; }

        /// <summary>
        /// Use Prod or Dev Clsids
        /// </summary>
        public bool UseDevClsids { init; get; }

        /// <summary>
        /// Create instance of the provided type.
        /// </summary>
        /// <typeparam name="T">Projected class type.</typeparam>
        /// <returns>Instance of the provided type.</returns>
        protected override T CreateInstanceInternal<T>()
        {
            var clsid = ClassesDefinition.GetClsid<T>(Context);
            var iid = ClassesDefinition.GetIid<T>();

            var instanceInPtr = ComUtils.CoCreateInstanceLocalServer(clsid, iid, AllowLowerTrustRegistration);
            return MarshalGeneric<T>.FromAbi(instanceInPtr);
        }
    }
}
