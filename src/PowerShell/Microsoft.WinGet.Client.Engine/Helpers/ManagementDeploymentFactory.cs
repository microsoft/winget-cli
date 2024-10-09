// -----------------------------------------------------------------------------
// <copyright file="ManagementDeploymentFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;

#if NET
    using WinRT;
#endif

    /// <summary>
    /// Constructs instances of classes from the <see cref="Management.Deployment" /> namespace.
    /// </summary>
    internal sealed class ManagementDeploymentFactory
    {
#if USE_PROD_CLSIDS
        private static readonly Guid PackageManagerClsid = Guid.Parse("C53A4F16-787E-42A4-B304-29EFFB4BF597");
        private static readonly Guid FindPackagesOptionsClsid = Guid.Parse("572DED96-9C60-4526-8F92-EE7D91D38C1A");
        private static readonly Guid CreateCompositePackageCatalogOptionsClsid = Guid.Parse("526534B8-7E46-47C8-8416-B1685C327D37");
        private static readonly Guid InstallOptionsClsid = Guid.Parse("1095F097-EB96-453B-B4E6-1613637F3B14");
        private static readonly Guid UninstallOptionsClsid = Guid.Parse("E1D9A11E-9F85-4D87-9C17-2B93143ADB8D");
        private static readonly Guid PackageMatchFilterClsid = Guid.Parse("D02C9DAF-99DC-429C-B503-4E504E4AB000");
        private static readonly Guid DownloadOptionsClsid = Guid.Parse("4CBABE76-7322-4BE4-9CEA-2589A80682DC");
        private static readonly Guid RepairOptionsClsid = Guid.Parse("0498F441-3097-455F-9CAF-148F28293865");
#else
        private static readonly Guid PackageManagerClsid = Guid.Parse("74CB3139-B7C5-4B9E-9388-E6616DEA288C");
        private static readonly Guid FindPackagesOptionsClsid = Guid.Parse("1BD8FF3A-EC50-4F69-AEEE-DF4C9D3BAA96");
        private static readonly Guid CreateCompositePackageCatalogOptionsClsid = Guid.Parse("EE160901-B317-4EA7-9CC6-5355C6D7D8A7");
        private static readonly Guid InstallOptionsClsid = Guid.Parse("44FE0580-62F7-44D4-9E91-AA9614AB3E86");
        private static readonly Guid UninstallOptionsClsid = Guid.Parse("AA2A5C04-1AD9-46C4-B74F-6B334AD7EB8C");
        private static readonly Guid PackageMatchFilterClsid = Guid.Parse("3F85B9F4-487A-4C48-9035-2903F8A6D9E8");
        private static readonly Guid DownloadOptionsClsid = Guid.Parse("8EF324ED-367C-4880-83E5-BB2ABD0B72F6");
        private static readonly Guid RepairOptionsClsid = Guid.Parse("E62BB1E7-C7B2-4AEC-9E28-FB649B30FF03");
#endif
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type? PackageManagerType = Type.GetTypeFromCLSID(PackageManagerClsid);
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type? FindPackagesOptionsType = Type.GetTypeFromCLSID(FindPackagesOptionsClsid);
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type? CreateCompositePackageCatalogOptionsType = Type.GetTypeFromCLSID(CreateCompositePackageCatalogOptionsClsid);
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type? InstallOptionsType = Type.GetTypeFromCLSID(InstallOptionsClsid);
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type? UninstallOptionsType = Type.GetTypeFromCLSID(UninstallOptionsClsid);
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type? PackageMatchFilterType = Type.GetTypeFromCLSID(PackageMatchFilterClsid);
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type? DownloadOptionsType = Type.GetTypeFromCLSID(DownloadOptionsClsid);
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static readonly Type? RepairOptionsType = Type.GetTypeFromCLSID(RepairOptionsClsid);

        // These GUIDs correspond to the CSWinRT interface IIDs generated for Microsoft.Management.Deployment.Projection
        // and are auto-generated by the WinRT tool.
        private static readonly Guid PackageManagerIid = Guid.Parse("B375E3B9-F2E0-5C93-87A7-B67497F7E593");
        private static readonly Guid FindPackagesOptionsIid = Guid.Parse("A5270EDD-7DA7-57A3-BACE-F2593553561F");
        private static readonly Guid CreateCompositePackageCatalogOptionsIid = Guid.Parse("21ABAA76-089D-51C5-A745-C85EEFE70116");
        private static readonly Guid InstallOptionsIid = Guid.Parse("6EE9DB69-AB48-5E72-A474-33A924CD23B3");
        private static readonly Guid UninstallOptionsIid = Guid.Parse("3EBC67F0-8339-594B-8A42-F90B69D02BBE");
        private static readonly Guid PackageMatchFilterIid = Guid.Parse("D981ECA3-4DE5-5AD7-967A-698C7D60FC3B");
        private static readonly Guid DownloadOptionsIid = Guid.Parse("94C92C4B-43F5-5CA3-BBBE-9F432C9546BC");
        private static readonly Guid RepairOptionsIid = Guid.Parse("263F0546-2D7E-53A0-B8D1-75B74817FF18");

        private static readonly IEnumerable<Architecture> ValidArchs = new Architecture[] { Architecture.X86, Architecture.X64 };

        private static readonly Lazy<ManagementDeploymentFactory> Lazy = new (() => new ManagementDeploymentFactory());

        /// <summary>
        /// Initializes static members of the <see cref="ManagementDeploymentFactory"/> class.
        /// </summary>
        static ManagementDeploymentFactory()
        {
            if (Utilities.UsesInProcWinget)
            {
                PackageManagerSettings settings = new PackageManagerSettings();
                settings.SetCallerIdentifier("PowerShellInProc");
            }
        }

        private ManagementDeploymentFactory()
        {
        }

        /// <summary>
        /// Gets the instance object.
        /// </summary>
        public static ManagementDeploymentFactory Instance
        {
            get { return Lazy.Value; }
        }

        /// <summary>
        /// Creates an instance of the <see cref="PackageManager" /> class.
        /// </summary>
        /// <returns>A <see cref="PackageManager" /> instance.</returns>
        public PackageManager CreatePackageManager()
        {
            return Create<PackageManager>(PackageManagerType, PackageManagerIid);
        }

        /// <summary>
        /// Creates an instance of the <see cref="FindPackagesOptions" /> class.
        /// </summary>
        /// <returns>A <see cref="FindPackagesOptions" /> instance.</returns>
        public FindPackagesOptions CreateFindPackagesOptions()
        {
            return Create<FindPackagesOptions>(FindPackagesOptionsType, FindPackagesOptionsIid);
        }

        /// <summary>
        /// Creates an instance of the <see cref="CreateCompositePackageCatalogOptions" /> class.
        /// </summary>
        /// <returns>A <see cref="CreateCompositePackageCatalogOptions" /> instance.</returns>
        public CreateCompositePackageCatalogOptions CreateCreateCompositePackageCatalogOptions()
        {
            return Create<CreateCompositePackageCatalogOptions>(CreateCompositePackageCatalogOptionsType, CreateCompositePackageCatalogOptionsIid);
        }

        /// <summary>
        /// Creates an instance of the <see cref="InstallOptions" /> class.
        /// </summary>
        /// <returns>An <see cref="InstallOptions" /> instance.</returns>
        public InstallOptions CreateInstallOptions()
        {
            return Create<InstallOptions>(InstallOptionsType, InstallOptionsIid);
        }

        /// <summary>
        /// Creates an instance of the <see cref="UninstallOptions" /> class.
        /// </summary>
        /// <returns>A <see cref="UninstallOptions" /> instance.</returns>
        public UninstallOptions CreateUninstallOptions()
        {
            return Create<UninstallOptions>(UninstallOptionsType, UninstallOptionsIid);
        }

        /// <summary>
        /// Creates an instance of the <see cref="DownloadOptions" /> class.
        /// </summary>
        /// <returns>A <see cref="DownloadOptions" /> instance.</returns>
        public DownloadOptions CreateDownloadOptions()
        {
            return Create<DownloadOptions>(DownloadOptionsType, DownloadOptionsIid);
        }

        /// <summary>
        /// Creates an instance of the <see cref="PackageMatchFilter" /> class.
        /// </summary>
        /// <returns>A <see cref="PackageMatchFilter" /> instance.</returns>
        public PackageMatchFilter CreatePackageMatchFilter()
        {
            return Create<PackageMatchFilter>(PackageMatchFilterType, PackageMatchFilterIid);
        }

        /// <summary>
        /// Creates an instance of the <see cref="RepairOptions" /> class.
        /// </summary>
        /// <returns>A <see cref="RepairOptions" /> instance.</returns>
        public RepairOptions CreateRepairOptions()
        {
            return Create<RepairOptions>(RepairOptionsType, RepairOptionsIid);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "COM only usage.")]
        private static T Create<T>(Type? type, in Guid iid)
            where T : new()
        {
            if (type == null)
            {
                throw new ArgumentNullException(iid.ToString());
            }

            if (Utilities.UsesInProcWinget)
            {
                // This doesn't work on Windows PowerShell
                // If we want to support it, we need something that loads the
                // Microsoft.Management.Deployment.dll for .NET framework as CsWinRT
                // does for .NET Core
                return new T();
            }

            object? instance = null;

            if (Utilities.ExecutingAsAdministrator)
            {
                int hr = WinRTHelpers.ManualActivation(type.GUID, iid, 0, out instance);

                if (hr < 0)
                {
                    if (hr == ErrorCode.FileNotFound || hr == ErrorCode.PackageNotRegisteredForUser)
                    {
                        throw new WinGetIntegrityException(IntegrityCategory.AppInstallerNotInstalled);
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

            if (instance == null)
            {
                throw new ArgumentNullException();
            }

#if NET
            IntPtr pointer = Marshal.GetIUnknownForObject(instance);
            return MarshalInterface<T>.FromAbi(pointer);
#else
            return (T)instance;
#endif
        }
    }
}
