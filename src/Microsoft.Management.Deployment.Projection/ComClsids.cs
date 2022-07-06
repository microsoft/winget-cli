namespace Microsoft.Management.Deployment.Projection
{
    using System;

    public static class ComClsids
    {
        public static class InProcess
        {
            public static Guid PackageManager => Guid.Parse("2DDE4456-64D9-4673-8F7E-A4F19A2E6CC3");
            public static Guid FindPackagesOptions => Guid.Parse("96B9A53A-9228-4DA0-B013-BB1B2031AB3D");
            public static Guid CreateCompositePackageCatalogOptions => Guid.Parse("768318A6-2EB5-400D-84D0-DF3534C30F5D");
            public static Guid InstallOptions => Guid.Parse("E2AF3BA8-8A88-4766-9DDA-AE4013ADE286");
            public static Guid UninstallOptions => Guid.Parse("869CB959-EB54-425C-A1E4-1A1C291C64E9");
            public static Guid PackageMatchFilter => Guid.Parse("57DC8962-7343-42CD-B91C-04F6A25DB1D0");
            public static Guid PackageManagerSettings => Guid.Parse("80CF9D63-5505-4342-B9B4-BB87895CA8BB");
        }

        // Non-prod (a.k.a WinGetDev)
        public static class OutOfProcess
        {
            public static Guid PackageManager => Guid.Parse("74CB3139-B7C5-4B9E-9388-E6616DEA288C");
            public static Guid FindPackagesOptions => Guid.Parse("1BD8FF3A-EC50-4F69-AEEE-DF4C9D3BAA96");
            public static Guid CreateCompositePackageCatalogOptions => Guid.Parse("EE160901-B317-4EA7-9CC6-5355C6D7D8A7");
            public static Guid InstallOptions => Guid.Parse("44FE0580-62F7-44D4-9E91-AA9614AB3E86");
            public static Guid UninstallOptions => Guid.Parse("AA2A5C04-1AD9-46C4-B74F-6B334AD7EB8C");
            public static Guid PackageMatchFilter => Guid.Parse("3F85B9F4-487A-4C48-9035-2903F8A6D9E8");
        }

        public static Guid GetCLSID<T>(bool inProc)
            where T : new()
        {
            return typeof(T) switch
            {
                Type t when t == typeof(PackageManager) => inProc ? InProcess.PackageManager : OutOfProcess.PackageManager,
                Type t when t == typeof(FindPackagesOptions) => inProc ? InProcess.FindPackagesOptions : OutOfProcess.FindPackagesOptions,
                Type t when t == typeof(CreateCompositePackageCatalogOptions) => inProc ? InProcess.CreateCompositePackageCatalogOptions : OutOfProcess.CreateCompositePackageCatalogOptions,
                Type t when t == typeof(InstallOptions) => inProc ? InProcess.InstallOptions : OutOfProcess.InstallOptions,
                Type t when t == typeof(UninstallOptions) => inProc ? InProcess.UninstallOptions : OutOfProcess.UninstallOptions,
                Type t when t == typeof(PackageMatchFilter) => inProc ? InProcess.PackageMatchFilter : OutOfProcess.PackageMatchFilter,
                Type t when t == typeof(PackageManagerSettings) => inProc ? InProcess.PackageManagerSettings : throw new InvalidOperationException($"{nameof(PackageManagerSettings)} cannot be used in out of process context"),
                _ => throw new InvalidOperationException($"{nameof(GetCLSID)} of {typeof(T).Name} is not supported")
            };
        }

        public static Guid GetIID<T>() where T : new()
        {
            return typeof(T) switch
            {
                Type t when t == typeof(PackageManager) => typeof(IPackageManager).GUID,
                Type t when t == typeof(FindPackagesOptions) => typeof(IFindPackagesOptions).GUID,
                Type t when t == typeof(CreateCompositePackageCatalogOptions) => typeof(ICreateCompositePackageCatalogOptions).GUID,
                Type t when t == typeof(InstallOptions) => typeof(IInstallOptions).GUID,
                Type t when t == typeof(UninstallOptions) => typeof(IUninstallOptions).GUID,
                Type t when t == typeof(PackageMatchFilter) => typeof(IPackageMatchFilter).GUID,
                Type t when t == typeof(PackageManagerSettings) => typeof(IPackageManagerSettings).GUID,
                _ => throw new InvalidOperationException($"{nameof(GetIID)} of {typeof(T).Name} is not supported")
            };
        }
    }
}