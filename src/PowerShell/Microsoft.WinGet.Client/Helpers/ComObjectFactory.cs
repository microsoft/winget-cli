// -----------------------------------------------------------------------------
// <copyright file="ComObjectFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Factories
{
    using System;
    using System.Runtime.InteropServices;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Exceptions;

#if NET
    using WinRT;
#endif

    /// <summary>
    /// Constructs instances of classes from the <see cref="Management.Deployment" /> namespace.
    /// </summary>
    public class ComObjectFactory
    {
#if USE_PROD_CLSIDS
        private static readonly Guid PackageManagerClsid = Guid.Parse("C53A4F16-787E-42A4-B304-29EFFB4BF597");
        private static readonly Guid FindPackagesOptionsClsid = Guid.Parse("572DED96-9C60-4526-8F92-EE7D91D38C1A");
        private static readonly Guid CreateCompositePackageCatalogOptionsClsid = Guid.Parse("526534B8-7E46-47C8-8416-B1685C327D37");
        private static readonly Guid InstallOptionsClsid = Guid.Parse("1095F097-EB96-453B-B4E6-1613637F3B14");
        private static readonly Guid UninstallOptionsClsid = Guid.Parse("E1D9A11E-9F85-4D87-9C17-2B93143ADB8D");
        private static readonly Guid PackageMatchFilterClsid = Guid.Parse("D02C9DAF-99DC-429C-B503-4E504E4AB000");
#else
        private static readonly Guid PackageManagerClsid = Guid.Parse("74CB3139-B7C5-4B9E-9388-E6616DEA288C");
        private static readonly Guid FindPackagesOptionsClsid = Guid.Parse("1BD8FF3A-EC50-4F69-AEEE-DF4C9D3BAA96");
        private static readonly Guid CreateCompositePackageCatalogOptionsClsid = Guid.Parse("EE160901-B317-4EA7-9CC6-5355C6D7D8A7");
        private static readonly Guid InstallOptionsClsid = Guid.Parse("44FE0580-62F7-44D4-9E91-AA9614AB3E86");
        private static readonly Guid UninstallOptionsClsid = Guid.Parse("AA2A5C04-1AD9-46C4-B74F-6B334AD7EB8C");
        private static readonly Guid PackageMatchFilterClsid = Guid.Parse("3F85B9F4-487A-4C48-9035-2903F8A6D9E8");
#endif
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type PackageManagerType = Type.GetTypeFromCLSID(PackageManagerClsid);
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type FindPackagesOptionsType = Type.GetTypeFromCLSID(FindPackagesOptionsClsid);
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type CreateCompositePackageCatalogOptionsType = Type.GetTypeFromCLSID(CreateCompositePackageCatalogOptionsClsid);
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type InstallOptionsType = Type.GetTypeFromCLSID(InstallOptionsClsid);
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type UninstallOptionsType = Type.GetTypeFromCLSID(UninstallOptionsClsid);
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type PackageMatchFilterType = Type.GetTypeFromCLSID(PackageMatchFilterClsid);

        private static readonly Guid PackageManagerIid = Guid.Parse("B375E3B9-F2E0-5C93-87A7-B67497F7E593");
        private static readonly Guid FindPackagesOptionsIid = Guid.Parse("A5270EDD-7DA7-57A3-BACE-F2593553561F");
        private static readonly Guid CreateCompositePackageCatalogOptionsIid = Guid.Parse("21ABAA76-089D-51C5-A745-C85EEFE70116");
        private static readonly Guid InstallOptionsIid = Guid.Parse("6EE9DB69-AB48-5E72-A474-33A924CD23B3");
        private static readonly Guid UninstallOptionsIid = Guid.Parse("3EBC67F0-8339-594B-8A42-F90B69D02BBE");
        private static readonly Guid PackageMatchFilterIid = Guid.Parse("D981ECA3-4DE5-5AD7-967A-698C7D60FC3B");

        /// <summary>
        /// Creates an instance of the <see cref="PackageManager" /> class.
        /// </summary>
        /// <returns>A <see cref="PackageManager" /> instance.</returns>
        public virtual PackageManager CreatePackageManager()
        {
            return Create<PackageManager>(PackageManagerType, PackageManagerIid);
        }

        /// <summary>
        /// Creates an instance of the <see cref="FindPackagesOptions" /> class.
        /// </summary>
        /// <returns>A <see cref="FindPackagesOptions" /> instance.</returns>
        public virtual FindPackagesOptions CreateFindPackagesOptions()
        {
            return Create<FindPackagesOptions>(FindPackagesOptionsType, FindPackagesOptionsIid);
        }

        /// <summary>
        /// Creates an instance of the <see cref="CreateCompositePackageCatalogOptions" /> class.
        /// </summary>
        /// <returns>A <see cref="CreateCompositePackageCatalogOptions" /> instance.</returns>
        public virtual CreateCompositePackageCatalogOptions CreateCreateCompositePackageCatalogOptions()
        {
            return Create<CreateCompositePackageCatalogOptions>(CreateCompositePackageCatalogOptionsType, CreateCompositePackageCatalogOptionsIid);
        }

        /// <summary>
        /// Creates an instance of the <see cref="InstallOptions" /> class.
        /// </summary>
        /// <returns>An <see cref="InstallOptions" /> instance.</returns>
        public virtual InstallOptions CreateInstallOptions()
        {
            return Create<InstallOptions>(InstallOptionsType, InstallOptionsIid);
        }

        /// <summary>
        /// Creates an instance of the <see cref="UninstallOptions" /> class.
        /// </summary>
        /// <returns>A <see cref="UninstallOptions" /> instance.</returns>
        public virtual UninstallOptions CreateUninstallOptions()
        {
            return Create<UninstallOptions>(UninstallOptionsType, UninstallOptionsIid);
        }

        /// <summary>
        /// Creates an instance of the <see cref="PackageMatchFilter" /> class.
        /// </summary>
        /// <returns>A <see cref="PackageMatchFilter" /> instance.</returns>
        public virtual PackageMatchFilter CreatePackageMatchFilter()
        {
            return Create<PackageMatchFilter>(PackageMatchFilterType, PackageMatchFilterIid);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static T Create<T>(Type type, in Guid iid)
        {
            object instance = null;

            if (Utilities.ExecutingAsAdministrator)
            {
                int hr = WinGetServerManualActivation_CreateInstance(type.GUID, iid, 0, out instance);

                if (hr < 0)
                {
                    if (hr == ErrorCode.FileNotFound)
                    {
                        throw new WinGetPackageNotInstalledException();
                    }
                    else
                    {
                        throw new COMException($"Failed to create instance: {hr}", hr);
                    }
                }
            }
            else
            {
                instance = Activator.CreateInstance(type);
            }

#if NET
            IntPtr pointer = Marshal.GetIUnknownForObject(instance);
            return MarshalInterface<T>.FromAbi(pointer);
#else
            return (T)instance;
#endif
        }

        [DllImport("winrtact.dll", EntryPoint = "WinGetServerManualActivation_CreateInstance", ExactSpelling = true, PreserveSig = true)]
        private static extern int WinGetServerManualActivation_CreateInstance(
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid clsid,
            [In, MarshalAs(UnmanagedType.LPStruct)] Guid iid,
            uint flags,
            [Out, MarshalAs(UnmanagedType.IUnknown)] out object instance);
    }
}
