// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.Management.Deployment.Projection
{
    using System;
    using System.Collections.Generic;

    internal static class ClassesDefinition
    {
        private static Dictionary<Type, ClassModel> Classes { get; set; } = new()
        {
            [typeof(PackageManager)] = new()
            {
                ProjectedClassType = typeof(PackageManager),
                InterfaceType = typeof(IPackageManager),
                Clsids = new Dictionary<ClsidContext, Guid>()
                {
                    [ClsidContext.InProc] = new Guid("2DDE4456-64D9-4673-8F7E-A4F19A2E6CC3"),
                    [ClsidContext.OutOfProc] = new Guid("C53A4F16-787E-42A4-B304-29EFFB4BF597"),
                    [ClsidContext.OutOfProcDev] = new Guid("74CB3139-B7C5-4B9E-9388-E6616DEA288C"),
                }
            },

            [typeof(FindPackagesOptions)] = new()
            {
                ProjectedClassType = typeof(FindPackagesOptions),
                InterfaceType = typeof(IFindPackagesOptions),
                Clsids = new Dictionary<ClsidContext, Guid>()
                {
                    [ClsidContext.InProc] = new Guid("96B9A53A-9228-4DA0-B013-BB1B2031AB3D"),
                    [ClsidContext.OutOfProc] = new Guid("572DED96-9C60-4526-8F92-EE7D91D38C1A"),
                    [ClsidContext.OutOfProcDev] = new Guid("1BD8FF3A-EC50-4F69-AEEE-DF4C9D3BAA96"),
                }
            },

            [typeof(CreateCompositePackageCatalogOptions)] = new()
            {
                ProjectedClassType = typeof(CreateCompositePackageCatalogOptions),
                InterfaceType = typeof(ICreateCompositePackageCatalogOptions),
                Clsids = new Dictionary<ClsidContext, Guid>()
                {
                    [ClsidContext.InProc] = new Guid("768318A6-2EB5-400D-84D0-DF3534C30F5D"),
                    [ClsidContext.OutOfProc] = new Guid("526534B8-7E46-47C8-8416-B1685C327D37"),
                    [ClsidContext.OutOfProcDev] = new Guid("EE160901-B317-4EA7-9CC6-5355C6D7D8A7"),
                }
            },

            [typeof(InstallOptions)] = new()
            {
                ProjectedClassType = typeof(InstallOptions),
                InterfaceType = typeof(IInstallOptions),
                Clsids = new Dictionary<ClsidContext, Guid>()
                {
                    [ClsidContext.InProc] = new Guid("E2AF3BA8-8A88-4766-9DDA-AE4013ADE286"),
                    [ClsidContext.OutOfProc] = new Guid("1095F097-EB96-453B-B4E6-1613637F3B14"),
                    [ClsidContext.OutOfProcDev] = new Guid("44FE0580-62F7-44D4-9E91-AA9614AB3E86"),
                }
            },

            [typeof(UninstallOptions)] = new()
            {
                ProjectedClassType = typeof(UninstallOptions),
                InterfaceType = typeof(IUninstallOptions),
                Clsids = new Dictionary<ClsidContext, Guid>()
                {
                    [ClsidContext.InProc] = new Guid("869CB959-EB54-425C-A1E4-1A1C291C64E9"),
                    [ClsidContext.OutOfProc] = new Guid("E1D9A11E-9F85-4D87-9C17-2B93143ADB8D"),
                    [ClsidContext.OutOfProcDev] = new Guid("AA2A5C04-1AD9-46C4-B74F-6B334AD7EB8C"),
                }
            },

            [typeof(DownloadOptions)] = new()
            {
                ProjectedClassType = typeof(DownloadOptions),
                InterfaceType = typeof(IDownloadOptions),
                Clsids = new Dictionary<ClsidContext, Guid>()
                {
                    [ClsidContext.InProc] = new Guid("4288DF96-FDC9-4B68-B403-193DBBF56A24"),
                    [ClsidContext.OutOfProc] = new Guid("4CBABE76-7322-4BE4-9CEA-2589A80682DC"),
                    [ClsidContext.OutOfProcDev] = new Guid("8EF324ED-367C-4880-83E5-BB2ABD0B72F6"),
                }
            },

            [typeof(PackageMatchFilter)] = new()
            {
                ProjectedClassType = typeof(PackageMatchFilter),
                InterfaceType = typeof(IPackageMatchFilter),
                Clsids = new Dictionary<ClsidContext, Guid>()
                {
                    [ClsidContext.InProc] = new Guid("57DC8962-7343-42CD-B91C-04F6A25DB1D0"),
                    [ClsidContext.OutOfProc] = new Guid("D02C9DAF-99DC-429C-B503-4E504E4AB000"),
                    [ClsidContext.OutOfProcDev] = new Guid("3F85B9F4-487A-4C48-9035-2903F8A6D9E8"),
                }
            },

            [typeof(AuthenticationArguments)] = new()
            {
                ProjectedClassType = typeof(AuthenticationArguments),
                InterfaceType = typeof(IAuthenticationArguments),
                Clsids = new Dictionary<ClsidContext, Guid>()
                {
                    [ClsidContext.InProc] = new Guid("8D593114-1CF1-43B9-8722-4DBB30103296"),
                    [ClsidContext.OutOfProc] = new Guid("BA580786-BDE3-4F6C-B8F3-44698AC8711A"),
                    [ClsidContext.OutOfProcDev] = new Guid("6484A61D-50FA-41F0-B71E-F4370C6EB37C"),
                }
            },

            [typeof(PackageManagerSettings)] = new()
            {
                ProjectedClassType = typeof(PackageManagerSettings),
                InterfaceType = typeof(IPackageManagerSettings),
                Clsids = new Dictionary<ClsidContext, Guid>()
                {
                    [ClsidContext.InProc] = new Guid("80CF9D63-5505-4342-B9B4-BB87895CA8BB"),
                }
            },

            [typeof(RepairOptions)] = new ()
            {
                ProjectedClassType = typeof(RepairOptions),
                InterfaceType = typeof(IRepairOptions),
                Clsids = new Dictionary<ClsidContext, Guid>()
                {
                    [ClsidContext.InProc] = new Guid("30C024C4-852C-4DD4-9810-1348C51EF9BB"),
                    [ClsidContext.OutOfProc] = new Guid("0498F441-3097-455F-9CAF-148F28293865"),
                    [ClsidContext.OutOfProcDev] = new Guid("E62BB1E7-C7B2-4AEC-9E28-FB649B30FF03"),
                }
            },

            [typeof(AddPackageCatalogOptions)] = new()
            {
                ProjectedClassType = typeof(AddPackageCatalogOptions),
                InterfaceType = typeof(IAddPackageCatalogOptions),
                Clsids = new Dictionary<ClsidContext, Guid>()
                {
                    [ClsidContext.InProc] = new Guid("24E6F1FA-E4C3-4ACD-965D-DF213FD58F15"),
                    [ClsidContext.OutOfProc] = new Guid("DB9D012D-00D7-47EE-8FB1-606E10AC4F51"),
                    [ClsidContext.OutOfProcDev] = new Guid("D58C7E4C-70E6-476C-A5D4-80341ED80252"),
                }
            },

            [typeof(RemovePackageCatalogOptions)] = new()
            {
                ProjectedClassType = typeof(RemovePackageCatalogOptions),
                InterfaceType = typeof(IRemovePackageCatalogOptions),
                Clsids = new Dictionary<ClsidContext, Guid>()
                {
                    [ClsidContext.InProc] = new Guid("1125D3A6-E2CE-479A-91D5-71A3F6F8B00B"),
                    [ClsidContext.OutOfProc] = new Guid("032B1C58-B975-469B-A013-E632B6ECE8D8"),
                    [ClsidContext.OutOfProcDev] = new Guid("87A96609-1A39-4955-BE72-7174E147B7DC"),
                }
            }
        };

        /// <summary>
        /// Get CLSID based on the provided context for the specified type
        /// </summary>
        /// <typeparam name="T">Projected class type</typeparam>
        /// <param name="context">Context</param>
        /// <returns>CLSID for the provided context and type, or throw an exception if not found.</returns>
        public static Guid GetClsid<T>(ClsidContext context)
        {
            ValidateType(typeof(T));
            return Classes[typeof(T)].GetClsid(context);
        }

        /// <summary>
        /// Get IID corresponding to the COM object
        /// </summary>
        /// <typeparam name="T">Projected class type</typeparam>
        /// <returns>IID or throw an exception if not found.</returns>
        public static Guid GetIid<T>()
        {
            ValidateType(typeof(T));
            return Classes[typeof(T)].GetIid();
        }

        /// <summary>
        /// Validate that the provided type is defined.
        /// </summary>
        /// <param name="type">Projected class type</param>
        /// <exception cref="InvalidOperationException"></exception>
        private static void ValidateType(Type type)
        {
            if (!Classes.ContainsKey(type))
            {
                throw new InvalidOperationException($"{type.Name} is not a projected class type.");
            }
        }
    }
}
