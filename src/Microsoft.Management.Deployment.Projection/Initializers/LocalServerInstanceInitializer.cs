// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.Management.Deployment.Projection
{
    using WinRT;

    // Out-of-process COM instance initializer.
    public class LocalServerInstanceInitializer : IInstanceInitializer
    {
        /// <summary>
        /// Out-of-process context.
        /// </summary>
        public ClsidContext Context => UseDevClsids ? ClsidContext.OutOfProcDev : ClsidContext.OutOfProc;

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
        public T CreateInstance<T>()
            where T : new()
        {
            var clsid = ClassesDefinition.GetClsid<T>(Context);
            var iid = ClassesDefinition.GetIid<T>();

            var instanceInPtr = ComUtils.CoCreateInstanceLocalServer(clsid, iid, AllowLowerTrustRegistration);
            return MarshalGeneric<T>.FromAbi(instanceInPtr);
        }
    }
}
