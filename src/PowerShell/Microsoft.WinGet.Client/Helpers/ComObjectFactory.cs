// -----------------------------------------------------------------------------
// <copyright file="ComObjectFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Factories
{
    using System;
    using Microsoft.Management.Deployment;

#if NET
    using System.Runtime.InteropServices;
    using WinRT;
#endif

    /// <summary>
    /// Constructs instances of classes from the <see cref="Management.Deployment" /> namespace.
    /// </summary>
    public class ComObjectFactory
    {
#if USE_TEST_CLSIDS
#else
        private static readonly Guid PackageManagerClsid = Guid.Parse("C53A4F16-787E-42A4-B304-29EFFB4BF597");
        private static readonly Guid FindPackagesOptionsClsid = Guid.Parse("572DED96-9C60-4526-8F92-EE7D91D38C1A");
        private static readonly Guid CreateCompositePackageCatalogOptionsClsid = Guid.Parse("526534B8-7E46-47C8-8416-B1685C327D37");
        private static readonly Guid InstallOptionsClsid = Guid.Parse("1095F097-EB96-453B-B4E6-1613637F3B14");
        private static readonly Guid UninstallOptionsClsid = Guid.Parse("E1D9A11E-9F85-4D87-9C17-2B93143ADB8D");
        private static readonly Guid PackageMatchFilterClsid = Guid.Parse("D02C9DAF-99DC-429C-B503-4E504E4AB000");
#endif

        private static readonly Type PackageManagerType = Type.GetTypeFromCLSID(PackageManagerClsid);
        private static readonly Type FindPackagesOptionsType = Type.GetTypeFromCLSID(FindPackagesOptionsClsid);
        private static readonly Type CreateCompositePackageCatalogOptionsType = Type.GetTypeFromCLSID(CreateCompositePackageCatalogOptionsClsid);
        private static readonly Type InstallOptionsType = Type.GetTypeFromCLSID(InstallOptionsClsid);
        private static readonly Type UninstallOptionsType = Type.GetTypeFromCLSID(UninstallOptionsClsid);
        private static readonly Type PackageMatchFilterType = Type.GetTypeFromCLSID(PackageMatchFilterClsid);

        /// <summary>
        /// Creates an instance of the <see cref="PackageManager" /> class.
        /// </summary>
        /// <returns>A <see cref="PackageManager" /> instance.</returns>
        public virtual PackageManager CreatePackageManager()
        {
            return Create<PackageManager>(PackageManagerType);
        }

        /// <summary>
        /// Creates an instance of the <see cref="FindPackagesOptions" /> class.
        /// </summary>
        /// <returns>A <see cref="FindPackagesOptions" /> instance.</returns>
        public virtual FindPackagesOptions CreateFindPackagesOptions()
        {
            return Create<FindPackagesOptions>(FindPackagesOptionsType);
        }

        /// <summary>
        /// Creates an instance of the <see cref="CreateCompositePackageCatalogOptions" /> class.
        /// </summary>
        /// <returns>A <see cref="CreateCompositePackageCatalogOptions" /> instance.</returns>
        public virtual CreateCompositePackageCatalogOptions CreateCreateCompositePackageCatalogOptions()
        {
            return Create<CreateCompositePackageCatalogOptions>(CreateCompositePackageCatalogOptionsType);
        }

        /// <summary>
        /// Creates an instance of the <see cref="InstallOptions" /> class.
        /// </summary>
        /// <returns>An <see cref="InstallOptions" /> instance.</returns>
        public virtual InstallOptions CreateInstallOptions()
        {
            return Create<InstallOptions>(InstallOptionsType);
        }

        /// <summary>
        /// Creates an instance of the <see cref="UninstallOptions" /> class.
        /// </summary>
        /// <returns>A <see cref="UninstallOptions" /> instance.</returns>
        public virtual UninstallOptions CreateUninstallOptions()
        {
            return Create<UninstallOptions>(UninstallOptionsType);
        }

        /// <summary>
        /// Creates an instance of the <see cref="PackageMatchFilter" /> class.
        /// </summary>
        /// <returns>A <see cref="PackageMatchFilter" /> instance.</returns>
        public virtual PackageMatchFilter CreatePackageMatchFilter()
        {
            return Create<PackageMatchFilter>(PackageMatchFilterType);
        }

        private static T Create<T>(Type type)
        {
            object instance = Activator.CreateInstance(type);
#if NET
            IntPtr pointer = Marshal.GetIUnknownForObject(instance);
            return MarshalInterface<T>.FromAbi(pointer);
#else
            return (T)instance;
#endif
        }
    }
}
