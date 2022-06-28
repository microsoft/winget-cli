using Microsoft.Management.Deployment;

namespace WinGetProjection
{
    public class WinGetProjectionFactory
    {

        public WinGetProjectionFactory(IInstanceInitializer instanceInitializer)
        {
            InstanceInitializer = instanceInitializer;
        }

        private IInstanceInitializer InstanceInitializer { get; set; }

        public PackageManager CreatePackageManager() => InstanceInitializer.CreateInstance<PackageManager>();

        public FindPackagesOptions CreateFindPackagesOptions() => InstanceInitializer.CreateInstance<FindPackagesOptions>();

        public CreateCompositePackageCatalogOptions CreateCreateCompositePackageCatalogOptions() => InstanceInitializer.CreateInstance<CreateCompositePackageCatalogOptions>();

        public InstallOptions CreateInstallOptions() => InstanceInitializer.CreateInstance<InstallOptions>();

        public UninstallOptions CreateUninstallOptions() => InstanceInitializer.CreateInstance<UninstallOptions>();

        public PackageMatchFilter CreatePackageMatchFilter() => InstanceInitializer.CreateInstance<PackageMatchFilter>();

        public PackageManagerSettings CreatePackageManagerSettings() => InstanceInitializer.CreateInstance<PackageManagerSettings>();
    }
}
